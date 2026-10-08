#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <QUuid>
#include <QVector>

namespace qcad_more {

// Value snapshots own their bytes; they never own or retain live MEntity pointers.
// The future codec must record its version and fix legacy Serialize() first.
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
};

// Future block entities persist this reference instead of duplicating definitions.
// Each successful insertion creates its own non-null instanceId.
struct BlockInstance {
    QUuid instanceId;
    QUuid definitionId;
    QPointF position;
};

} // namespace qcad_more
