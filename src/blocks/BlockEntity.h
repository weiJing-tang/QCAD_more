#pragma once
#include "qcad/ENTITY.H"
#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"
#include <memory>
#include <vector>
namespace qcad_more {
class BlockEntity final : public MEntity {
public:
    BlockInstance instance;
    std::vector<std::unique_ptr<MEntity>> children;
    BlockEntity() { Init(); m_type = etBlock; }
    MEntity* Copy() override;
    void Draw(QCADView* view, int mode = dmNormal) override;
    bool Pick(const QPointF& point, double radius) override;
    void GetBox(QRectF& box) override;
    void Move(const QPointF& from, const QPointF& to, bool temporary = false) override;
};
OperationResult makeBlockEntity(const BlockDefinition& definition, const BlockInstance& instance,
                                std::unique_ptr<BlockEntity>& output);
const BlockDefinition* findBlock(const BlockLibrary& library, const QUuid& id);
}
