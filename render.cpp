#include "render.hpp"

void drawUnit(const RenderEntry& e) {
    switch (e.type) {
        case UnitType::Circle:
            DrawCircleV({e.x, e.y}, e.r, e.color);
            DrawCircleLines((int)e.x, (int)e.y, e.r, Fade(WHITE, 0.12f));
            break;
        case UnitType::Square:
            DrawRectangle((int)(e.x - e.r), (int)(e.y - e.r), (int)(e.r * 2), (int)(e.r * 2), e.color);
            DrawRectangleLines((int)(e.x - e.r), (int)(e.y - e.r), (int)(e.r * 2), (int)(e.r * 2), Fade(WHITE, 0.12f));
            break;
        case UnitType::Triangle: {
            Vector2 v1{e.x, e.y - e.r}, v2{e.x - e.r, e.y + e.r}, v3{e.x + e.r, e.y + e.r};
            DrawTriangle(v1, v2, v3, e.color);
            break;
        }
    }

    // Barre de HP : vert > 50 %, jaune > 25 %, rouge sinon
    float pct = (float)e.hp / e.maxHp;
    int bx = (int)(e.x - 12), by = (int)(e.y - e.r - 7);
    DrawRectangle(bx, by, 24, 3, DARKGRAY);
    DrawRectangle(bx, by, (int)(24 * pct), 3,
                  pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED));
}

void drawUnitIcon(float cx, float cy, float r, UnitType t, Color c) {
    switch (t) {
        case UnitType::Circle:   DrawCircleV({cx, cy}, r, c); break;
        case UnitType::Square:   DrawRectangle((int)(cx - r), (int)(cy - r), (int)(r * 2), (int)(r * 2), c); break;
        case UnitType::Triangle: {
            Vector2 v1{cx, cy - r}, v2{cx - r, cy + r}, v3{cx + r, cy + r};
            DrawTriangle(v1, v2, v3, c);
            break;
        }
    }
}