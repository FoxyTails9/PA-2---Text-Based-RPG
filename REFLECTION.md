1. For every function you wrote (TODO 1 to 19), explain in your own words what it does and give its time complexity in terms of n, the number of scenes. One short paragraph each.

TODO 1, Scene constructor. If the scene has more than 0 choices, it allocates exactly `numChoices_` slots with new Scene<T>*[numChoices_] and sets every slot to nullptr. If the scene is an ending, it allocates nothing and `choices_` stays nullptr. Time: O(1), because a scene has at most 3 slots.

TODO 2, Scene destructor. It loops over the slots and deletes each child scene, which makes every child run its own destructor, so the whole subtree is freed. Then it deletes the array with delete[] and finally deletes the payload. Time: O(n) when called on the root, since every scene is deleted exactly once.

TODO 3, isEnding. It returns true when `numChoices_` is 0. Time: O(1).

TODO 4, getChoice. It returns nullptr if the index is below 0 or at least `numChoices_`. Otherwise, it returns the scene in that slot. Time: O(1).

TODO 5, setChoice. It does nothing if the index is not a real slot. Otherwise, it stores the scene in that slot. Time: O(1).

TODO 6, countScenes. It returns 0 for nullptr. Otherwise, it returns 1 for the current scene plus the recursive count of each child. Time: O(n), because it visits each scene once.

TODO 7, countEndings. It returns 0 for nullptr and 1 for an ending. Otherwise, it adds up the recursive counts of the children. Time: O(n).

TODO 8, height. It returns -1 for nullptr. For any other scene it takes the tallest child height and adds 1. An ending has no children, so its height comes out as 0. Time: O(n).

TODO 9, clear. It deletes the root, which frees the whole story through the Scene destructor, and then sets the root to nullptr. Time: O(n).

TODO 10, load. It opens the file and prints the exact error if the file will not open. Then it reads the START_HP line and converts it with std::stoi. Next it builds the story with readScene and gives the root to the tree. It returns false on any failure. Time: O(n), because each line is read once.

TODO 11, readScene. It reads the five fields in order. If CHOICES is below 0 or above 3, it prints the exact error and returns nullptr before allocating anything. Otherwise it builds a Scene with a new Moment. For each choice it reads a LABEL line and then calls itself to read the whole child scene. If any step fails it deletes the scene, which frees the children already attached, and returns nullptr. Time: O(n), because each scene is read once.

TODO 12, askChoice. It prints "Your choice: " and reads a whole line with getline. It accepts the line only if it is all digits and the number is between 1 and the number of choices. Anything else prints the reprompt and asks again. It returns 0 if the input ends. Time: O(1) per attempt.

TODO 13, play. It starts at the root with the starting HP, 0 loot and an empty bag. At each scene it adds the HP change and loot, prints the dashes and the text, and handles the item. It then prints the status line and stops if HP is 0 or less or the scene is an ending. Otherwise it prints the choices and moves to the one askChoice returns. At the end it prints the final stats and the bag. Time: it follows one path from the root, and each item check costs O(m) for m items in the bag, so O(n^2) in the worst case. In my story that is at most 6 scenes and 2 items.

TODO 14, printStats. It prints the three numbers from countScenes, countEndings and height. Time: O(n).

TODO 15, printSpoilers. It calls printEndings on the root with an empty path. Time: O(n).

TODO 16, printEndings. It does nothing for nullptr. At an ending it prints the path and the scene text. Otherwise it loops over the choices in order and recurses with the path extended by that choice number, so choice 1 prints before choice 2 before choice 3. Time: O(n).

TODO 17, greedyRun. It enters the root, then loops. It stops if HP is 0 or less (died) or the scene is an ending. Otherwise it looks at the loot of each child scene, takes the largest (ties go to the lower choice number), extends the path and enters that scene. It never looks past the next scene and never checks HP before choosing. Time: O(h), where h is the height, because it follows one path and checks at most 3 children per step.

TODO 18, bestLoot. It returns -1 for nullptr. It applies the scene's HP change and returns -1 if HP drops to 0 or less. At an ending it saves the path and returns the scene's loot. Otherwise it tries every child, keeps the best result (ties go to the lower choice number), and returns this scene's loot plus the best child, or -1 if every child is a dead end. Time: O(n).

TODO 19, optimalRun. It calls bestLoot on the root with the starting HP and an empty path. It prints the path, loot and result, or the "no path survives" message if the answer is -1. Time: O(n).

2. Why does each scene allocate exactly numChoices slots instead of declaring Scene<T>* choices[3]? What does an ending hold in choices_, and why?

A scene with some number of choices allocates exactly that many slots instead of using Scene<T>* choices[3]. A fixed array of 3 would give all 17 scenes 3 slots each, 51 in total. My story only has 16 real choices, and my 7 endings need no slots at all. Exact size also means `numChoices_` is always the true number of slots, so getChoice and setChoice can check the index against it and isEnding is simply a check for 0.

An ending holds nullptr in `choices_`. I ask for 0 slots, so I never allocate an array and there is nothing to free. Calling delete[] on nullptr in the destructor is safe and does nothing.

3. Walk through your Scene destructor. What gets deleted, and in what order? What would go wrong if choices_ were deleted first?

My destructor deletes the child scenes first, through a loop over the slots. Each child runs its own destructor, so the whole subtree below is freed. Then it deletes the array itself, and last it deletes the payload.

If the array were deleted first, the child pointers stored in its slots would be gone. I could no longer reach the children, so every scene below this one would leak. Looping over the slots afterward would read freed memory, which is undefined behavior. The order of the payload relative to the children does not matter, because they are separate allocations.

4. Show the path greedy took and the path optimal took on your story, with the loot for each. Explain why greedy lost. If greedy did not lose, change your story until it does, and explain what you changed.

Greedy took the path 2-2-1 (east wing, ballroom, look up) for 7 loot and died. Optimal took the path 3-1-1 (west wing, bedroom, escape) for 15 loot and reached Ending 7.

Greedy lost because it only compares the loot of the next scene. At the start the east wing (3 loot) beats the annex (2) and the west wing (1). At the east wing the ballroom (4) beats the library (2). The ballroom's only choice leads to Ending 6, where the HP change of -10 kills the hero (10 - 1 - 10 = -1). Greedy never checks HP and never looks past the next scene, so it walked into a trap that looked like treasure. Optimal tries every path. The west wing starts with the least loot (1), but it leads to the bedroom (8) and Ending 7 (6) for a total of 15, and the hero survives with 4 HP.

Greedy lost the first time, so I did not need to change my story.

5. bestLoot tries every path, yet it runs in O(n) time. Explain why it is not exponential, using what you know about paths and leaves in a tree.

bestLoot tries every path, but in a tree each scene has exactly one parent, so there is exactly one path to each scene. The number of root-to-ending paths equals the number of endings, which is at most n. In my story that is 7 paths. There are also no overlapping subproblems. Every call works on a different scene, so the function is called at most once per scene (17 calls for my story), and each call does constant work because a scene has at most 3 children. That adds up to O(n). A recursion becomes exponential when the same subproblem is solved again and again, and a tree never revisits a scene.

6. Which List methods does your game use, and why? What does one search cost as the bag grows?

The game uses four List methods. addFront adds a newly picked-up item to the bag in O(1), and the bag takes ownership of the new Item. search checks whether the bag already has an item with the same name before adding it. It walks the list from the head, so one search costs O(m) for m items, and it gets slower as the bag grows. In my story the largest bag on any path is 2 items, so it stays cheap. size prints the item count in the "Inventory" line, and print prints the bag, one item per line.

The list's destructor runs automatically when play returns and frees every item, so nothing leaks. In my story all item names are different, so "You already have" never appears.
