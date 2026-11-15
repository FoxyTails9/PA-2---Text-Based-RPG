# PA2 Reflection: Story Tree

In this file, **n** is the number of scenes in the story. My own story, *Haunted Mansion*, has n = 17 scenes, 7 endings, and a height of 5.

## 1. What each function does and its time complexity

**TODO 1, `Scene` constructor.** If the scene has `numChoices_ > 0`, it allocates exactly `numChoices_` slots with `new Scene<T>*[numChoices_]` and sets every slot to `nullptr`. If the scene is an ending, it allocates nothing and `choices_` stays `nullptr`. Time: O(1), because a scene has at most 3 slots.

**TODO 2, `Scene` destructor.** It loops over the slots and deletes each child scene, which makes every child run its own destructor, so the whole subtree is freed. Then it runs `delete[] choices_` and finally `delete data_`. Time: O(n) when called on the root, since every scene is deleted exactly once. For a single scene it is O(1) plus its children.

**TODO 3, `isEnding`.** It returns `numChoices_ == 0`. Time: O(1).

**TODO 4, `getChoice`.** It returns `nullptr` if the index is below 0 or at least `numChoices_`. Otherwise it returns `choices_[index]`. Time: O(1).

**TODO 5, `setChoice`.** It does nothing if the index is not a real slot. Otherwise it stores `next` in `choices_[index]`. Time: O(1).

**TODO 6, `countScenes`.** It returns 0 for `nullptr`. Otherwise it returns 1 for the current scene plus the recursive count of each child. Time: O(n), because it visits each scene once.

**TODO 7, `countEndings`.** It returns 0 for `nullptr` and 1 for an ending. Otherwise it adds up the recursive counts of the children. Time: O(n).

**TODO 8, `height`.** It returns -1 for `nullptr`. For any other scene it takes the tallest child height and adds 1. An ending has no children, so the tallest stays at -1 and the scene's height becomes 0. Time: O(n).

**TODO 9, `clear`.** It runs `delete root_`, which frees the whole story through the `Scene` destructor, and then sets `root_ = nullptr`. Time: O(n).

**TODO 10, `load`.** It opens the file and prints the exact error if the file will not open. Then it reads the `START_HP` line with `readField` and converts it with `std::stoi`. Next it builds the story with `readScene(in, "START")` and gives the root to `tree_.setRoot`. It returns false on any failure. Time: O(n), because each line is read once and each scene has five lines plus its choice labels.

**TODO 11, `readScene`.** It reads the five fields in order. If `CHOICES` is outside 0 to 3, it prints the exact error and returns `nullptr` before allocating anything. Otherwise it builds a `Scene<Moment>` with a new `Moment`. For each choice it reads a `LABEL` line and then calls itself to read the whole child scene. If any step fails it runs `delete scene`, which frees the children already attached, and returns `nullptr`. Time: O(n), because each scene is read once.

**TODO 12, `askChoice`.** It prints `Your choice: ` and reads a whole line with `std::getline`. It trims spaces and accepts the line only if it is all digits, short enough that `stoi` cannot overflow, and in the range 1 to `numChoices`. Anything else prints the reprompt and asks again. It returns 0 if the input ends. Time: O(1) per attempt, since a line is only a few characters.

**TODO 13, `play`.** It starts at the root with `hp = startHp_`, `loot = 0` and an empty `List<Item>` bag. At each scene it adds the HP change and loot, prints the dashes and the text, and handles the item. It then prints the status line and stops on `hp <= 0` or on an ending. Otherwise it prints the choices and moves to the one `askChoice` returns. It ends by printing the final stats and the bag. Time: it visits one root-to-ending path, so at most height + 1 scenes, and each item check costs O(m) for m items in the bag. That is O(n²) in the worst case, but at most 6 scenes and 2 items in my story.

**TODO 14, `printStats`.** It prints the three numbers from `countScenes`, `countEndings` and `height`. Time: O(n), since it runs three O(n) traversals.

**TODO 15, `printSpoilers`.** It calls `printEndings` on the root with an empty path. Time: O(n).

**TODO 16, `printEndings`.** It does nothing for `nullptr`. At an ending it prints `Path <path>: <text>`, using `(start)` if the path is empty. Otherwise it loops over the choices in order and recurses with `extendPath(path, i + 1)`, so choice 1 prints before choice 2 before choice 3. Time: O(n) scenes visited, plus the cost of copying the path string, which is at most the height.

**TODO 17, `greedyRun`.** It enters the root, then loops. It stops if `hp <= 0` (died) or the scene is an ending. Otherwise it looks at the loot of each child scene, takes the largest with a strict `>` so ties go to the lower number, extends the path and enters that scene. It never looks past the next scene and never checks HP before choosing. Time: O(h), where h is the height, because it follows one path and checks at most 3 children per step.

**TODO 18, `bestLoot`.** It returns -1 for `nullptr`. It applies the scene's HP change and returns -1 if `hp <= 0`. At an ending it saves the path and returns the scene's loot. Otherwise it tries every child, keeps the best result (a strict `>` keeps the lower choice number on a tie), and returns this scene's loot plus the best child, or -1 if every child is a dead end. Time: O(n).

**TODO 19, `optimalRun`.** It calls `bestLoot` on the root with `startHp_` and an empty path. It prints the path, loot and result, or the "no path survives" message if the answer is -1. Time: O(n).

## 2. Why exact-size allocation?

A scene with `numChoices` choices allocates exactly that many slots instead of `Scene<T>* choices[3]`. A fixed array of 3 would give all 17 scenes 3 slots each, 51 in total. My story only has 16 real choices (the number of branches in a tree of 17 scenes), and my 7 endings need no slots at all. Exact size also means `numChoices_` is always the true number of slots, so `getChoice` and `setChoice` can bounds-check against it and `isEnding` is simply `numChoices_ == 0`.

An ending holds `choices_ == nullptr`. I ask for 0 slots, so I never call `new[]` and there is no array to free. The destructor's `delete[] choices_` on `nullptr` is safe and does nothing.

## 3. The `Scene` destructor

My destructor deletes things in this order:

1. Every child scene, through a loop over `choices_[i]`. Each child runs its own destructor, so the whole subtree below is freed.
2. The array itself, with `delete[] choices_`.
3. The payload, with `delete data_`.

If `choices_` were deleted first, the child pointers stored in its slots would be gone. I could no longer reach the children, so every scene below this one would leak. Looping over the slots afterward would read freed memory, which is undefined behavior. The order of `data_` relative to the children does not matter, because they are separate allocations.

## 4. Greedy vs optimal on *Haunted Mansion*

| Advisor | Path | Loot | Result |
|---|---|---|---|
| Greedy | `2-2-1` (east wing, ballroom, look up) | 7 | died |
| Optimal | `3-1-1` (west wing, bedroom, escape) | 15 | reached an ending (Ending 7) |

Greedy lost because it only compares the loot of the next scene. At the start the east wing (3) beats the annex (2) and the west wing (1). At the east wing the ballroom (4) beats the library (2). The ballroom's only choice leads to Ending 6, where the HP change of -10 kills the hero (10 - 1 - 10 = -1). Greedy never checks HP and never looks past the next scene, so it walked into a trap that looked like treasure. Optimal tries every path: the west wing starts with the least loot (1), but it leads to the bedroom (8) and Ending 7 (6) for a total of 15, and the hero survives with 4 HP.

Greedy lost the first time, so I did not need to change my story.

## 5. Why `bestLoot` is not exponential

`bestLoot` tries every path, but in a tree each scene has exactly one parent, so there is exactly one path to each scene. The number of root-to-ending paths equals the number of endings, which is at most n. In my story that is 7 paths. There are also no overlapping subproblems. Every call works on a different scene, so the function is called at most once per scene (17 calls for my story), and each call does constant work because a scene has at most 3 children. That adds up to O(n). A recursion becomes exponential when the same subproblem is solved again and again, and a tree never revisits a scene.

## 6. Which `List` methods the game uses

- **`addFront`:** adds a newly picked-up item to the bag in O(1). The bag takes ownership of the `new Item`.
- **`search`:** checks whether the bag already has an item with the same name before adding it, using `Item::operator==`. A scan of m items costs O(m), because it walks the list from the head. The bag only grows by one item per scene, so a search is cheap in practice, but it gets slower as the bag grows. In my story the largest bag on any path is 2 items.
- **`size`:** prints the item count in `Inventory (<count> items):`.
- **`print`:** prints the bag, one item per line.

The list's destructor, which calls `deleteFront` until the list is empty, runs automatically when `play` returns. That frees every item, so nothing leaks. In my story all item names are different, so "You already have" never appears.
