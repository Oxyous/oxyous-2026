//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_DATASTRUCTURES_HPP
#define OXYOUS_2026_DATASTRUCTURES_HPP

#include "includes.hpp"

class Renderer;

struct saveState{

};

/* App Engine Struct */
struct appEngine{
    int32_t width;
    int32_t height;
    struct android_app* app;
    struct saveState* state;
    Renderer* renderer;
};

/* GPU Image Struct */
typedef struct GPUImage {
    VkFormat format;
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;
} GPUImage;



#endif //OXYOUS_2026_DATASTRUCTURES_HPP
