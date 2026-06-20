//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GPUMESH_HPP
#define OXYOUS_2026_GPUMESH_HPP

class GPUMesh {
public:
    GPUMesh() = default;
    virtual ~GPUMesh() = default;
public:
    /* */
    virtual void render() = 0;
};

#endif //OXYOUS_2026_GPUMESH_HPP
