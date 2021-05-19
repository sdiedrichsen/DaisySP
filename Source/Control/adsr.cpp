#include "adsr.h"
#include <math.h>

using namespace daisysp;


void Adsr::Init(float sample_rate, int blockSize)
{
    sample_rate_ = sample_rate / blockSize;
    SetTime(ADSR_SEG_ATTACK, 0.1f);
    SetTime(ADSR_SEG_DECAY, 0.1f);
    SetTime(ADSR_SEG_RELEASE, 0.1f);
    sus_level_ = 0.7f;
    x_         = 0.0f;
}

void Adsr::Retrigger(bool hard)
{
    mode_ = ADSR_SEG_ATTACK;
    if(hard)
      x_ = 0.f;
}

void Adsr::SetTime(int seg, float time)
{
    if(seg_time_[seg] != time)
    {
        seg_time_[seg] = time;
        if(time > 0.f)
        {
            const float attackTarget = logf(0.5f);
            const float decayTarget  = logf(1. / M_E);
            float       target
                = (seg == ADSR_SEG_ATTACK) ? attackTarget : decayTarget;
            seg_D0_[seg] = 1.f - expf(target / (time * sample_rate_));            
        }
        else
            seg_D0_[seg] = 1.f; // instant change
    }
}


float Adsr::Process(bool gate)
{
    float out = 0.0f;

    if(gate && mode_ != ADSR_SEG_DECAY)
        mode_ = ADSR_SEG_ATTACK;
    else if(!gate && mode_ != ADSR_SEG_IDLE)
        mode_ = ADSR_SEG_RELEASE;

    float D0(seg_D0_[mode_]);
    float target = mode_ == ADSR_SEG_DECAY ? sus_level_ : - 0.1f;
    switch(mode_)
    {
        case ADSR_SEG_IDLE: out = 0.0f; break;
        case ADSR_SEG_ATTACK:
            x_ += D0 * (2.f - x_);
            out = x_;
            if(out > 1.f)
            {
                x_  = out = 1.f;
                mode_     = ADSR_SEG_DECAY;
            }
            break;
        case ADSR_SEG_DECAY:
        case ADSR_SEG_RELEASE:
            x_ += D0 * (target - x_);
            out = x_;
            if(out < 0.0f)
            {
                x_ = out = 0.f;
                mode_    = ADSR_SEG_IDLE;
            }
        default: break;
    }
    return out;
}
