#ifndef GAME_COMPONENTS_H
#define GAME_COMPONENTS_H

// Components
struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
};

struct Speed {
    float value = 0.0f;
};

// Tamaño y forma
struct Radius {
    float value = 0.0f;
};

struct Size2D {
    float width = 0.0f;
    float height = 0.0f;
};

// Gameplay
struct Score {
    int value = 0;
};

// Etiquetas (Tags) para identificar entidades
struct PlayerTag {};
struct PaddleTag {};
struct BallTag {};

#endif // GAME_COMPONENTS_H
