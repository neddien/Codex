#include "Phonograph.h"

void PhonographController::OnInit()
{
    m_Asc = &GetComponent<AudioSourceComponent>();

    // auto& asc = GetComponent<AudioSourceComponent>();
    //  asc.handle->Play();
    /*if (m_Asc)
        m_Asc->handle->Play();*/

    // ax::EventHandle h = GetAudioEvent("event:/Phonograph-music-2");
    // h.Play();
}

void PhonographController::OnUpdate(const f32 deltaTime)
{
}

void PhonographController::OnFixedUpdate(const f32 deltaTime)
{
}
