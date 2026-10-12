#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <QUuid>
#include <QVector>

namespace qcad_more {

// Value snapshots own their bytes; they never own or retain live MEntity pointers.
// Payloads use EntityCodec and QDataStream::Qt_5_12; v2 supports recursive blocks.
struct EntitySnapshot {
    int entityType = 0; // Maps to EEntityType in the course project's ENTITY.H.
    QByteArray payload;
};

struct BlockDefinition {
    QUuid id;
    QString name;
    QPointF sourceBasePoint; // World point chosen when defining the block.
    QVector<EntitySnapshot> entities; // Geometry relative to sourceBasePoint.
};

struct BlockLibrary {
    QVector<BlockDefinition> definitions; // Unique, non-null IDs and unique names.
};

struct DefineBlockRequest {
    QString name;
    QPointF basePoint; // World coordinates; source geometry is kept unchanged.
};

struct InsertBlockRequest {
    QUuid definitionId;
    QPointF position; // World coordinates to which the block origin is mapped.
    double rotationDegrees = 0;
    double scale = 1;
};

// Instance metadata accompanies a private recursive snapshot in document v2.
// Each successful insertion creates its own non-null instanceId.
struct BlockInstance {
    QUuid instanceId;
    QUuid definitionId;
    QPointF position;
    double rotationDegrees = 0;
    double scale = 1;
};

} // namespace qcad_more
