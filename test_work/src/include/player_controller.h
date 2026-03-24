#include <codex.h>

using namespace codex;

class player_controller : public NativeBehaviour
{
private:
    TransformComponent* trans = nullptr;
    Sprite*             sprite;
    f32                 velocity = 180.f;

public:
    void OnInit() override
    {
        using namespace codex;

        std::cout << "player_controller initialized.\n";
        trans  = &GetComponent<TransformComponent>();
        sprite = &GetComponent<SpriteRendererComponent>().GetSprite();
    }
    void OnUpdate(const f32 deltaTime) override
    {
        using namespace codex;

        if (Input::IsKeyDown(Key::W))
        {
            trans->position.y -= velocity * deltaTime;
        }
        if (Input::IsKeyDown(Key::A))
        {
            trans->position.x -= velocity * deltaTime;
        }
        if (Input::IsKeyDown(Key::S))
        {
            trans->position.y += velocity * deltaTime;
        }
        if (Input::IsKeyDown(Key::D))
        {
            trans->position.x += velocity * deltaTime;
        }
        if (Input::IsKeyDown(Key::Up))
        {
            trans->scale.x += 1.0f * deltaTime;
            trans->scale.y += 1.0f * deltaTime;
        }
        if (Input::IsKeyDown(Key::Down))
        {
            trans->scale.x -= 1.0f * deltaTime;
            trans->scale.y -= 1.0f * deltaTime;
        }
    }
};
