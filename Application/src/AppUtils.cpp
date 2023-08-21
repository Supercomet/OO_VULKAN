#include "AppUtils.h"

#include "MeshModel.h"
#include "BoudingVolume.h"
#include "Vulkanrenderer.h"

#include <random>

bool BoolQueryUser(const char* str)
{
    char response{ 0 };
    std::cout << str << " [y/n]" << std::endl;
    while (!response)
    {
        std::cin >> response;
        response = static_cast<char>(std::tolower(response));
        if (response != 'y' && response != 'n')
        {
            std::cout << "Invalid input [" << response << "] Please try again" << std::endl;
            response = 0;
        }
    }
    return response == 'n' ? false : true;
}

oGFX::Color GenerateRandomColor()
{
    static std::default_random_engine rndEngine(3456);
    static std::uniform_real<float> uniformDist(0.0f, 1.0f);

    oGFX::Color col;
    col.a = 1.0f;
    float sum;
    do
    {
        col.r = uniformDist(rndEngine);
        col.g = uniformDist(rndEngine);
        col.b = uniformDist(rndEngine);
        sum = (col.r + col.g + col.b);
    } while (sum < 2.0f && sum > 2.8f);
    return col;
}

void UpdateBV(ModelFileResource* model, VulkanRenderer::EntityDetails& entity, int i)
{
    std::vector<oGFX::Point3D> vertPositions;
    vertPositions.resize(model->vertices.size());
    glm::mat4 xform(1.0f);
    xform = glm::translate(xform, entity.position);
    xform = glm::rotate(xform, glm::radians(entity.rot), entity.rotVec);
    xform = glm::scale(xform, entity.scale);
    std::transform(model->vertices.begin(), model->vertices.end(), vertPositions.begin(), [&](const oGFX::Vertex& v) {
        glm::vec4 pos{ v.pos,1.0f };
        pos = xform * pos;
        return pos;
        }
    );
    switch (i)
    {
    case 0:
        oGFX::BV::RitterSphere(entity.sphere, vertPositions);
        break;
    case 1:
        oGFX::BV::LarsonSphere(entity.sphere, vertPositions, oGFX::BV::EPOS::_6);
        break;
    case 2:
        oGFX::BV::LarsonSphere(entity.sphere, vertPositions, oGFX::BV::EPOS::_14);
        break;
    case 3:
        oGFX::BV::LarsonSphere(entity.sphere, vertPositions, oGFX::BV::EPOS::_26);
        break;
    case 4:
        oGFX::BV::LarsonSphere(entity.sphere, vertPositions, oGFX::BV::EPOS::_98);
        break;
    case 5:
        oGFX::BV::EigenSphere(entity.sphere, vertPositions);
        break;
    default:
        oGFX::BV::RitterSphere(entity.sphere, vertPositions);
        break;
    }

    oGFX::BV::BoundingAABB(entity.aabb, vertPositions);
}
