<p align="center">
  <img src="indus-dragon-icon.webp" alt="Indus Dragon" width="150" style="border-radius: 25px;">
</p>
<h1 align="center">Indus Dragon Chess Engine</h1>

Indus Dragon is a UCI chess engine written in C++ that uses the [chess-library](https://github.com/Disservin/chess-library) for board operations and move validation.

[Join our Discord Channel](https://discord.gg/ZBW5DBw8)

[Play the engine](https://lichess.org/@/indusdragon)

## Building the Project

### Prerequisites

- CMake (version 3.15 or higher)
- A C++ compiler that supports C++17
- Git (optional, only needed if you want to clone the repository)

### Build Instructions

1. Clone the repository (if you haven't already):

```sh
git clone https://github.com/Razamindset/indus-dragon
cd indus-dragon
```

2. Create a build directory and navigate to it:

```sh
mkdir build
cd build
```

3. Configure and build the project:

```sh
cmake ..
cmake --build .
```

The executable will be created in the `build` directory.

## Running the Engine

After building, you can run the engine from the build directory:

```sh
./indus-dragon
```

**⚠️ Important Notice ⚠️**

The engine has been updated to use extended search to reach quiet positions howevver a lot of work is reamaining. the engine also has a simple time management where it uses 1/20 of remaining time to find the best move which needs to be made more dynamic. Also, The next steps will be improving perf and evaluation.

---

## Future Plans

- Beat stockfish

## Attribution Notices

Indus Dragon uses:

- [chess-library](https://github.com/Disservin/chess-library) for board representation
- Techniques and algorithms from [Chess Programming Wiki](https://www.chessprogramming.org)

Stay tuned for updates!
