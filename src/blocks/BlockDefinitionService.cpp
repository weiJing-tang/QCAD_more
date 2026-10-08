#include "blocks/BlockDefinitionService.h"
#include "storage/EntityCodec.h"
#include <QSet>
namespace qcad_more {
OperationResult BlockDefinitionService::defineBlock(const QList<MEntity*>& selected,
    const DefineBlockRequest& request, BlockLibrary& library, QUuid& definitionId) const
{
    if (selected.isEmpty()) return {ErrorCode::EmptySelection, QStringLiteral("请先选择图形。")};
    const QString name = request.name.trimmed();
    if (name.isEmpty() || !finitePoint(request.basePoint))
        return {ErrorCode::InvalidInput, QStringLiteral("块名或基点无效。")};
    for (const auto& definition : library.definitions)
        if (definition.name == name)
            return {ErrorCode::DuplicateBlockName, QStringLiteral("块名已存在，请使用其他名称。")};
    BlockDefinition definition;
    definition.id = QUuid::createUuid();
    definition.name = name;
    definition.sourceBasePoint = request.basePoint;
    QSet<MEntity*> seen;
    for (auto* entity : selected) {
        if (!entity) return {ErrorCode::InvalidInput, QStringLiteral("选择集包含无效图元。")};
        if (seen.contains(entity)) continue;
        seen.insert(entity);
        EntitySnapshot snapshot;
        auto result = encodeEntity(*entity, snapshot);
        if (!result.ok()) return result;
        std::unique_ptr<MEntity> copy;
        result = decodeEntity(snapshot, copy);
        if (!result.ok()) return result;
        copy->Move(request.basePoint, QPointF());
        result = encodeEntity(*copy, snapshot);
        if (!result.ok()) return result;
        definition.entities.push_back(std::move(snapshot));
    }
    library.definitions.push_back(definition);
    definitionId = definition.id;
    return {};
}
}
