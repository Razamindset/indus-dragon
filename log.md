## Add PVS

Today on 3 july 2025 I added PVS to our chess engine. This made improvement to the over all game play of the engine.
It won against the older version as both black and white. And I expect a good elo gain. I have included comments in the code explaining the ideas. If needed i will add some explaination here too.

## Removed PVS

After repeated testing seems like there is some issue that needs to be solved before we can use PVS.

## NPS, MATE Issue, Repeating.

@tissatussa shared some games that point out that engine missed a mate and drew a game in winning position. This could be realted to PVS or TT. But we need to test it after recent changes before I can proceed.

## Refactor and bug detection

On 19 july 2025 I reafctored the entire codebase, separating the different parts into their own classes so in future it is easy to handle. But there is still a lot of work.

It seems like there was malicious code in the write storeTT function that was causing severe issues when in end game and table is full so for now I am using simply a vector for this purpose lets see how it behaves. Honestly I didnot write that code, my implementation was simple. I was testing gemini cli earlier with this project, that might have put that code here.
After 2 3 games looks like the bug is gone also note that for the current approach to work the size of tt must be a power of 2 for bitwise operations to work when storing and searching. I will write more about this when i feel this approach works for us.

## Refactor Use negamax

The negamax works similar to min max but the code is shorte. We take each player as maximizing and switch the alpha and beta values on each call. Also we add perspective to the evaluation value. As the search resolves we get the same result as min max but with less code. Date: 21 july 2025.

## Add NNUE based evalauation

Have added a nnue file that will be used for board evaluation instead of a handcrafted function.
For using the nnue I am using the probe nnue library that gives bataries to load and evaluate positions. https://github.com/dshawul/nnue-probe. The library currently only supports a specific 20MB nnue. In future i plan to expand it so i can load multi nnues or different networks.
The nnue file is from some old stockfish version. I am actively working on training custom networks for indus dragon but it will take quite some time to get right cause turns out its not very straight forward. There are many constraints at hand like data, training resources, Until then I will focus more on refactoring the code and improving the search.

## Full Code Refactor

Dated: 30th august 2025
I refactored a lot of code from my chess engine mostly related to the uci adapter. The goal was to some what simplify the code so it is more understandable and performant. Many files were removed and added...
Commit: 6a95e7a
