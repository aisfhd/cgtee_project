#include "physics.h"
#include "../other/globals.h"
#include "worldgen.h"
void checkCollisions(glm::vec3 &pos, float &velY) {
    int bX = (int)round(pos.x / BLOCK_SCALE);
    int bZ = (int)round(pos.z / BLOCK_SCALE);
    int h = getWorldHeight((float)bX, (float)bZ);
    float groundHeight = h * BLOCK_SCALE + (BLOCK_SCALE / 2.0f) + playerHeight;
    
    if (pos.y < groundHeight) {
        if (velY <= 0) {
            pos.y = groundHeight;
            velY = 0;
            isGrounded = true;
        }
    } else {
        isGrounded = false;
    }
}