#include "phonograph.h"

void PhonographController::on_init()
{
    asc_ = &get_component<AudioSourceComponent>();

    // auto& asc = get_component<AudioSourceComponent>();
    //  asc.handle->play();
    /*if (asc_)
        asc_->handle->play();*/

    // ax::EventHandle h = get_audio_event("event:/Phonograph-music-2");
    // h.play();
}

void PhonographController::on_update(const f32 delta_time)
{
}

void PhonographController::on_fixed_update(const f32 delta_time)
{
}
