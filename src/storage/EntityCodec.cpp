#include "storage/EntityCodec.h"
#include <QBuffer>
#include <cmath>
namespace qcad_more {
bool finitePoint(const QPointF& p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }
namespace {
std::unique_ptr<MEntity> makeEntity(int type)
{
    switch (type) {
    case etLine: return std::make_unique<MLine>();
    case etRectangle: return std::make_unique<MRectangle>();
    case etCircle: return std::make_unique<CCircle>();
    case etArc: return std::make_unique<CArc>();
    case etEllipse: return std::make_unique<MEllipse>();
    case etPolygon: return std::make_unique<MPolygon>();
    case etText: return std::make_unique<MText>();
    default: return {};
    }
}
}
OperationResult encodeEntity(MEntity& entity, EntitySnapshot& output)
{
    if (!makeEntity(entity.GetType()))
        return {ErrorCode::UnsupportedEntity, QStringLiteral("该图元暂不支持；块内不能再包含块。")};
    EntitySnapshot snapshot;
    snapshot.entityType = entity.GetType();
    QDataStream stream(&snapshot.payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    entity.Serialize(stream, true);
    if (stream.status() != QDataStream::Ok)
        return {ErrorCode::IoError, QStringLiteral("图元编码失败。")};
    output = std::move(snapshot);
    return {};
}
OperationResult decodeEntity(const EntitySnapshot& snapshot, std::unique_ptr<MEntity>& output)
{
    auto entity = makeEntity(snapshot.entityType);
    if (!entity) return {ErrorCode::UnsupportedEntity, QStringLiteral("文件包含不支持的图元。")};
    QDataStream stream(snapshot.payload);
    stream.setVersion(QDataStream::Qt_5_12);
    entity->Serialize(stream, false);
    QRectF box;
    entity->GetBox(box);
    if (stream.status() != QDataStream::Ok || !stream.atEnd() ||
        !finitePoint(box.topLeft()) || !finitePoint(box.bottomRight()))
        return {ErrorCode::InvalidFormat, QStringLiteral("图元数据损坏或不完整。")};
    output = std::move(entity);
    return {};
}
}
