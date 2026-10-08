#pragma once
#include "blocks/BlockTypes.h"
#include "common/OperationResult.h"
#include "qcad/ENTITY.H"
#include <memory>
namespace qcad_more {
OperationResult encodeEntity(MEntity& entity, EntitySnapshot& output);
OperationResult decodeEntity(const EntitySnapshot& snapshot, std::unique_ptr<MEntity>& output);
bool finitePoint(const QPointF& point);
}
