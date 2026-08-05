#pragma once

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};