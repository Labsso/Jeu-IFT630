#pragma once

enum class UnitType  { Circle, Triangle, Square };
enum class UnitState { Marching, Attacking };

struct Unit {
    float     x = 0, y = 0;
    int       hp = 0, maxHp = 0;
    bool      alive   = false;
    bool      isEnemy = false;
    UnitType  type    = UnitType::Circle;
    UnitState state   = UnitState::Marching;
    int       target  = -1;     // index dans le pool de la cible d'attaque actuelle, -1 = aucune
    int       atkTimer = 0;     // ticks avant que la prochaine attaque soit autorisée
    float     laneY   = 0.0f;  // position verticale assignée au spawn

    float radius() const {
        switch (type) {
            case UnitType::Square:   return 13.0f;
            case UnitType::Triangle: return 8.0f;
            default:                 return 10.0f;
        }
    }
    int initHp() const {
        switch (type) {
            case UnitType::Square:   return 25;
            case UnitType::Triangle: return 6;
            default:                 return 12;
        }
    }
    float speed() const {
        switch (type) {
            case UnitType::Triangle: return 2.2f;
            case UnitType::Square:   return 0.7f;
            default:                 return 1.4f;
        }
    }
    int damage() const {
        switch (type) {
            case UnitType::Triangle: return 5;
            case UnitType::Square:   return 2;
            default:                 return 3;
        }
    }
    // Les carrés absorbent 2 dégâts ; les triangles percent l'armure entièrement.
    int armor() const { return type == UnitType::Square ? 2 : 0; }

    int atkCooldownMax() const {
        switch (type) {
            case UnitType::Square:   return 45;
            case UnitType::Triangle: return 20;
            default:                 return 30;
        }
    }
    int cost() const {
        switch (type) {
            case UnitType::Triangle: return 20;
            case UnitType::Square:   return 30;
            default:                 return 10;
        }
    }
};