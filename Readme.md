<p align="center">
  <img src="indus-dragon.png" alt="Indus Dragon" width="150">
</p>

# Indus Dragon Chess Engine

Indus Dragon is a UCI chess engine written in C++ that uses the [chess-library](https://github.com/Disservin/chess-library) for board operations and move validation.

[Join our Discord Channel](https://discord.gg/ZBW5DBw8)

## Play The engine

You can play the engine on lichess. If it is online.

```sh
https://lichess.org/@/indusdragon
```

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

The engine does not use quiesc search because the evaluation function is still in development. Quiesc search will be added soon. For now The goal is to craft the evaluation fucntion and add iterative deepening with time management.

---

## Future Plans

- Beat stockfish

## Attribution Notices

Indus Dragon uses:

- [chess-library](https://github.com/Disservin/chess-library) for board representation
- Techniques and algorithms from [Chess Programming Wiki](https://www.chessprogramming.org)

Stay tuned for updates!
