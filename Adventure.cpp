#include "Adventure.h"
#include <iostream>

// File-local helper: prints text word-wrapped so no line is longer than
// WRAP_WIDTH. A line that already fits is printed exactly as it is.
// firstPrefix starts the first line; indent starts every wrapped line.
namespace {
const int WRAP_WIDTH = 80;

void printWrapped(const std::string& text, const std::string& firstPrefix, const std::string& indent) {
    if ((int)(firstPrefix.size() + text.size()) <= WRAP_WIDTH) {
        std::cout << firstPrefix << text << std::endl;
        return;
    }
    std::string line = firstPrefix;
    bool lineHasWord = false;
    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && text[pos] == ' ') {
            pos++;
        }
        if (pos >= text.size()) {
            break;
        }
        size_t end = text.find(' ', pos);
        if (end == std::string::npos) {
            end = text.size();
        }
        std::string word = text.substr(pos, end - pos);
        pos = end;
        if (lineHasWord && (int)(line.size() + 1 + word.size()) > WRAP_WIDTH) {
            std::cout << line << std::endl;
            line = indent;
            lineHasWord = false;
        }
        if (lineHasWord) {
            line += " ";
        }
        line += word;
        lineHasWord = true;
    }
    std::cout << line << std::endl;
}
}

// ===================== GIVEN HELPERS (do not change) =====================

// Reads the next line that is not blank.
// Spaces at the start of the line are removed.
// Returns false when the file has no more lines.
bool Adventure::nextLine(std::ifstream& in, std::string& line) {
    std::string raw;
    while (std::getline(in, raw)) {
        int start = 0;
        while (start < (int)raw.size() && (raw[start] == ' ' || raw[start] == '\t')) {
            start++;
        }
        line = raw.substr(start);
        // Files saved on Windows can end each line with an extra '\r'.
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        if (!line.empty()) {
            return true;
        }
    }
    return false;
}

// Reads the next line and checks that it starts with keyword.
// Example: for the line "HP -2" and keyword "HP", value becomes "-2".
// Prints an error and returns false if the line is missing or wrong.
bool Adventure::readField(std::ifstream& in, const std::string& keyword, std::string& value) {
    std::string line;
    if (!nextLine(in, line)) {
        std::cout << "Error: expected a " << keyword << " line but the file ended." << std::endl;
        return false;
    }
    std::string prefix = keyword + " ";
    if (line.substr(0, prefix.size()) != prefix) {
        std::cout << "Error: expected a " << keyword << " line but found: " << line << std::endl;
        return false;
    }
    value = line.substr(prefix.size());
    return true;
}

// Adds one choice number to a path.
// extendPath("", 2) gives "2". extendPath("2-1", 3) gives "2-1-3".
std::string Adventure::extendPath(const std::string& path, int choiceNumber) const {
    if (path.empty()) {
        return std::to_string(choiceNumber);
    }
    return path + "-" + std::to_string(choiceNumber);
}

// ===================== LOADING =====================

// TODO 10: open the story file and build the tree.
//   - If the file will not open, print exactly:
//       Error: could not open file <fileName>
//     and return false.
//   - Read the START_HP line and save the number in startHp_.
//     (std::stoi turns "10" into 10.)
//   - Build the whole story with readScene. The first scene's label is "START".
//   - If anything failed, return false. Otherwise give the story to tree_.
bool Adventure::load(const std::string& fileName) {
    std::ifstream in(fileName);
    if (!in.is_open()) {
        std::cout << "Error: could not open file " << fileName << std::endl;
        return false;
    }

    std::string value;
    if (!readField(in, "START_HP", value)) {
        return false;
    }
    startHp_ = std::stoi(value);

    Scene<Moment>* root = readScene(in, "START");
    if (root == nullptr) {
        return false;   // readScene already printed the error and freed its work
    }
    tree_.setRoot(root);   // the tree now owns the whole story
    return true;
}

// TODO 11: read ONE scene and everything below it. Return it.
//   - Read SCENE, HP, LOOT, ITEM, and CHOICES with readField, in that order.
//   - If CHOICES is below 0 or above Scene<Moment>::MAX_CHOICES, print exactly:
//       Error: scene "<scene text>" has <n> choices. The limit is 3.
//     and return nullptr.
//   - Each choice is a LABEL line followed by a whole scene.
//   - If anything goes wrong at any depth, return nullptr
//     and make sure nothing you already built is leaked.
Scene<Moment>* Adventure::readScene(std::ifstream& in, const std::string& label) {
    std::string text, hpText, lootText, itemText, choicesText;
    if (!readField(in, "SCENE", text)) return nullptr;
    if (!readField(in, "HP", hpText)) return nullptr;
    if (!readField(in, "LOOT", lootText)) return nullptr;
    if (!readField(in, "ITEM", itemText)) return nullptr;
    if (!readField(in, "CHOICES", choicesText)) return nullptr;

    int hpChange = std::stoi(hpText);
    int loot = std::stoi(lootText);
    int numChoices = std::stoi(choicesText);

    const int limit = Scene<Moment>::MAX_CHOICES;
    if (numChoices < 0 || numChoices > limit) {
        std::cout << "Error: scene \"" << text << "\" has " << numChoices
                  << " choices. The limit is " << limit << "." << std::endl;
        return nullptr;   // nothing allocated yet for this scene, nothing to free
    }

    // From here on the scene owns the Moment, so one delete cleans up both.
    Scene<Moment>* scene = new Scene<Moment>(new Moment(text, label, hpChange, loot, itemText), numChoices);

    for (int i = 0; i < numChoices; i++) {
        std::string childLabel;
        if (!readField(in, "LABEL", childLabel)) {
            delete scene;   // frees the children already attached
            return nullptr;
        }
        Scene<Moment>* child = readScene(in, childLabel);   // the whole subtree
        if (child == nullptr) {
            delete scene;   // frees the children already attached
            return nullptr;
        }
        scene->setChoice(i, child);
    }
    return scene;
}

// ===================== PLAYING =====================

// TODO 12: ask the player to pick a choice from 1 to numChoices.
//   - Print "Your choice: " (no newline) and read a whole line.
//   - If the line is not a valid number in range, print exactly:
//       Please type a number from 1 to <numChoices>.
//     and ask again.
//   - If the input ends (getline fails), return 0.
int Adventure::askChoice(int numChoices) {
    std::string line;
    while (true) {
        std::cout << "Your choice: ";
        if (!std::getline(std::cin, line)) {
            return 0;   // input ended
        }

        // Trim spaces, tabs and a stray '\r' from both ends.
        int start = 0;
        int end = (int)line.size();
        while (start < end && (line[start] == ' ' || line[start] == '\t')) start++;
        while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r')) end--;
        std::string word = line.substr(start, end - start);

        // Only digits, and short enough that stoi cannot overflow.
        bool valid = !word.empty() && word.size() <= 9;
        for (int i = 0; valid && i < (int)word.size(); i++) {
            if (word[i] < '0' || word[i] > '9') {
                valid = false;
            }
        }
        if (valid) {
            int number = std::stoi(word);
            if (number >= 1 && number <= numChoices) {
                return number;
            }
        }
        std::cout << "Please type a number from 1 to " << numChoices << "." << std::endl;
    }
}

// TODO 13: play the story once from the start.
// Every time the player ENTERS a scene, in this order:
//   1. Add its HP change to hp and its loot to loot.
//   2. Print a line of 40 dashes, then the scene text.
//   3. If the scene has an item:
//        if the bag already has it, print "You already have: <item>"
//        otherwise add it to the bag and print "You picked up: <item>"
//   4. Print "HP: <hp>  Loot: <loot>" (two spaces before Loot).
//   5. If hp is 0 or less, print "Your HP hit 0. GAME OVER." and stop.
//      If the scene is an ending, print "THE END." and stop.
//   6. Print each choice as "[1] <label>" and use askChoice.
//      If askChoice returns 0, leave play right away.
// When the game stops, print:
//   Final HP: <hp>  Final loot: <loot>
//   Inventory (<count> items):
// and then print the bag.
// The bag is a List<Item>. Start with HP = startHp_ and loot = 0.
void Adventure::play() {
    Scene<Moment>* current = tree_.getRoot();
    if (current == nullptr) {
        return;   // no story loaded
    }

    int hp = startHp_;
    int loot = 0;
    List<Item> bag;

    while (current != nullptr) {
        Moment* moment = current->getData();

        // 1. Enter the scene.
        hp += moment->getHpChange();
        loot += moment->getLoot();

        // 2. Header and story text.
        std::cout << std::string(40, '-') << std::endl;
        printWrapped(moment->getText(), "", "");

        // 3. Item.
        if (moment->hasItem()) {
            Item found(moment->getItem());
            if (bag.search(found)) {
                std::cout << "You already have: " << moment->getItem() << std::endl;
            } else {
                bag.addFront(new Item(moment->getItem()));   // the bag now owns it
                std::cout << "You picked up: " << moment->getItem() << std::endl;
            }
        }

        // 4. Status line.
        std::cout << "HP: " << hp << "  Loot: " << loot << std::endl;

        // 5. Game over or ending.
        if (hp <= 0) {
            std::cout << "Your HP hit 0. GAME OVER." << std::endl;
            break;
        }
        if (current->isEnding()) {
            std::cout << "THE END." << std::endl;
            break;
        }

        // 6. Offer the choices.
        int numChoices = current->getNumChoices();
        for (int i = 0; i < numChoices; i++) {
            std::cout << "[" << (i + 1) << "] " << current->getChoice(i)->getData()->getLabel() << std::endl;
        }
        int picked = askChoice(numChoices);
        if (picked == 0) {
            break;   // input ended
        }
        current = current->getChoice(picked - 1);
    }

    std::cout << "Final HP: " << hp << "  Final loot: " << loot << std::endl;
    std::cout << "Inventory (" << bag.size() << " items):" << std::endl;
    bag.print();
}

// ===================== ANALYZING =====================

// TODO 14: print exactly three lines:
//   Scenes: <count>
//   Endings: <count>
//   Longest playthrough: <height> choices
void Adventure::printStats() const {
    std::cout << "Scenes: " << tree_.countScenes() << std::endl;
    std::cout << "Endings: " << tree_.countEndings() << std::endl;
    std::cout << "Longest playthrough: " << tree_.height() << " choices" << std::endl;
}

// TODO 15: print every ending in the story, one per line.
void Adventure::printSpoilers() const {
    printEndings(tree_.getRoot(), "");
}

// TODO 16: print every ending below scene.
// path holds the choice numbers taken to reach scene, like "2-1".
// Each ending prints as:
//   Path <path>: <scene text>
// If the very first scene is an ending, print "Path (start): <scene text>".
// Endings print in order: choice 1 before choice 2 before choice 3.
void Adventure::printEndings(Scene<Moment>* scene, const std::string& path) const {
    if (scene == nullptr) {
        return;
    }
    if (scene->isEnding()) {
        printWrapped(scene->getData()->getText(),
                     "Path " + (path.empty() ? std::string("(start)") : path) + ": ", "    ");
         std::cout << std::endl;
        return;
    }
    for (int i = 0; i < scene->getNumChoices(); i++) {
        printEndings(scene->getChoice(i), extendPath(path, i + 1));
    }
}

// TODO 17: the greedy advisor.
// Start at the first scene with startHp_ and 0 loot.
// Enter scenes the same way play does (add HP and loot).
// Stop when hp is 0 or less, or you reach an ending.
// Otherwise always move to the choice with the MOST loot right now.
// On a tie, pick the lower choice number.
// Print:
//   Greedy advisor
//     Path: <path>
//     Loot: <loot>
//     Result: died               (or "Result: reached an ending")
void Adventure::greedyRun() const {
    Scene<Moment>* current = tree_.getRoot();
    if (current == nullptr) {
        return;
    }

    int hp = startHp_;
    int loot = 0;
    std::string path = "";
    bool died = false;

    // Enter the first scene.
    hp += current->getData()->getHpChange();
    loot += current->getData()->getLoot();

    while (true) {
        if (hp <= 0) {
            died = true;
            break;
        }
        if (current->isEnding()) {
            break;
        }

        // Look only at the loot of each next scene. Strict > keeps the lower
        // choice number on a tie.
        int bestIndex = 0;
        int bestLootNow = current->getChoice(0)->getData()->getLoot();
        for (int i = 1; i < current->getNumChoices(); i++) {
            int candidate = current->getChoice(i)->getData()->getLoot();
            if (candidate > bestLootNow) {
                bestLootNow = candidate;
                bestIndex = i;
            }
        }

        path = extendPath(path, bestIndex + 1);
        current = current->getChoice(bestIndex);
        hp += current->getData()->getHpChange();
        loot += current->getData()->getLoot();
    }

    std::cout << "Greedy advisor" << std::endl;
    std::cout << "  Path: " << (path.empty() ? "(start)" : path) << std::endl;
    std::cout << "  Loot: " << loot << std::endl;
    std::cout << "  Result: " << (died ? "died" : "reached an ending") << std::endl;
}

// TODO 18: the best total loot you can collect from scene down to an ending,
// arriving at scene with hp health and having taken path to get here.
//   - Return -1 if every way down from here kills you.
//   - Put the path of the best run into bestPath.
//   - Must be recursive and look at every choice.
int Adventure::bestLoot(Scene<Moment>* scene, int hp, const std::string& path, std::string& bestPath) const {
    if (scene == nullptr) {
        return -1;
    }

    // Enter this scene.
    Moment* moment = scene->getData();
    int hpNow = hp + moment->getHpChange();
    if (hpNow <= 0) {
        return -1;   // this scene kills you
    }
    int lootHere = moment->getLoot();

    if (scene->isEnding()) {
        bestPath = path;
        return lootHere;
    }

    // Try every choice. Strict > keeps the lower choice number on a tie.
    int bestBelow = -1;
    std::string bestBelowPath;
    for (int i = 0; i < scene->getNumChoices(); i++) {
        std::string childBestPath;
        int result = bestLoot(scene->getChoice(i), hpNow, extendPath(path, i + 1), childBestPath);
        if (result > bestBelow) {
            bestBelow = result;
            bestBelowPath = childBestPath;
        }
    }

    if (bestBelow < 0) {
        return -1;   // every way down is a dead end
    }
    bestPath = bestBelowPath;
    return lootHere + bestBelow;
}

// TODO 19: the optimal advisor. Use bestLoot from the first scene.
// Print:
//   Optimal advisor
//     Path: <path>
//     Loot: <loot>
//     Result: reached an ending
// If no path survives, print only:
//   Optimal advisor
//     Result: no path survives this story
void Adventure::optimalRun() const {
    std::string bestPath;
    int best = bestLoot(tree_.getRoot(), startHp_, "", bestPath);

    std::cout << "Optimal advisor" << std::endl;
    if (best < 0) {
        std::cout << "  Result: no path survives this story" << std::endl;
        return;
    }
    std::cout << "  Path: " << (bestPath.empty() ? "(start)" : bestPath) << std::endl;
    std::cout << "  Loot: " << best << std::endl;
    std::cout << "  Result: reached an ending" << std::endl;
}
