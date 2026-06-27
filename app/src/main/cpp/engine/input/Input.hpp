//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_INPUT_HPP
#define OXYOUS_2026_INPUT_HPP


class IInput {
public:
    IInput() = default;
    virtual ~IInput() = default;
public:
    virtual void handleInput() = 0;
};

/* System input */
class Input : public IInput {
public:
    Input() = default;
    virtual ~Input() = default;
public:
    void handleInput() override;
};

#endif //OXYOUS_2026_INPUT_HPP
