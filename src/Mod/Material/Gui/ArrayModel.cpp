/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QList>
#include <QMetaType>
#endif

#include <Base/Console.h>
#include <Gui/MainWindow.h>
#include <Gui/MetaTypes.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "ArrayModel.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ArrayModel */

AbstractArrayModel::AbstractArrayModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

//===


Array2DModel::Array2DModel(const std::shared_ptr<Materials::MaterialProperty>& property,
                           const std::shared_ptr<Materials::Array2D>& value,
                           QObject* parent)
    : AbstractArrayModel(parent)
    , _property(property)
    , _value(value)
{}

int Array2DModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;  // No children
    }

    return _value->rows() + 1;  // Will always have 1 empty row
}

bool Array2DModel::newRow(const QModelIndex& index) const
{
    return (index.row() == _value->rows());
}

void Array2DModel::deleteRow(const QModelIndex& index)
{
    removeRows(index.row(), 1);
    Q_EMIT dataChanged(index, index);
}

int Array2DModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return _property->columns();
}

QVariant Array2DModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        try {
            return _value->getValue(index.row(), index.column());
        }
        catch (const Materials::InvalidIndex&) {
        }

        try {
            auto column = _property->getColumnType(index.column());
            if (column == Materials::MaterialValue::Quantity) {
                Base::Quantity qq = Base::Quantity(0, _property->getColumnUnits(index.column()).toStdString());
                qq.setFormat(Materials::MaterialValue::getQuantityFormat());
                return QVariant::fromValue(qq);
            }
        }
        catch (const Materials::InvalidIndex&) {
        }

        return QString();
    }

    return {};
}

QVariant Array2DModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            const Materials::MaterialProperty& column = _property->getColumn(section);
            return column.getDisplayName();
        }
        else if (orientation == Qt::Vertical) {
            // Vertical header
            if (section == (rowCount() - 1)) {
                return QStringLiteral("*");
            }
            return {section + 1};
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool Array2DModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);

    if (index.row() == _value->rows()) {
        insertRows(index.row(), 1);
    }
    _value->setValue(index.row(), index.column(), value);

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array2DModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array2DModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    int columns = columnCount();
    for (int i = 0; i < count; i++) {
        auto rowPtr = std::make_shared<QList<QVariant>>();
        for (int j = 0; j < columns; j++) {
            rowPtr->push_back(_property->getColumnNull(j));
        }

        _value->insertRow(row, rowPtr);
    }

    endInsertRows();

    return false;
}

bool Array2DModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        _value->deleteRow(row);
    }

    endRemoveRows();

    return false;
}

bool Array2DModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array2DModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

//===

Array3DDepthModel::Array3DDepthModel(const std::shared_ptr<Materials::MaterialProperty>& property,
                                     const std::shared_ptr<Materials::Array3D>& value,
                                     QObject* parent)
    : AbstractArrayModel(parent)
    , _property(property)
    , _value(value)
{}

int Array3DDepthModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;  // No children
    }

    return _value->depth() + 1;  // Will always have 1 empty row
}

bool Array3DDepthModel::newRow(const QModelIndex& index) const
{
    return (index.row() == _value->depth());
}

void Array3DDepthModel::deleteRow(const QModelIndex& index)
{
    removeRows(index.row(), 1);
    Q_EMIT dataChanged(index, index);
}

QVariant Array3DDepthModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        try {
            Base::Quantity qq = _value->getDepthValue(index.row());
            return QVariant::fromValue(qq);
        }
        catch (const Materials::InvalidIndex&) {
        }

        try {
            Base::Quantity qq = Base::Quantity(0, _property->getColumnUnits(0).toStdString());
            qq.setFormat(Materials::MaterialValue::getQuantityFormat());
            return QVariant::fromValue(qq);
        }
        catch (const Materials::InvalidIndex&) {
        }
    }

    return {};
}

QVariant Array3DDepthModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            const Materials::MaterialProperty& column = _property->getColumn(section);
            return column.getDisplayName();
        }
        if (orientation == Qt::Vertical) {
            // Vertical header
            if (section == (rowCount() - 1)) {
                return QStringLiteral("*");
            }
            return {section + 1};
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool Array3DDepthModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);

    if (index.row() == _value->depth()) {
        insertRows(index.row(), 1);
        _value->setCurrentDepth(index.row());
    }
    _value->setDepthValue(index.row(), value.value<Base::Quantity>());

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array3DDepthModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array3DDepthModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        auto qq = Base::Quantity(0, _property->getColumnUnits(0).toStdString());
        qq.setFormat(Materials::MaterialValue::getQuantityFormat());
        _value->addDepth(row, qq);
    }

    endInsertRows();

    return false;
}

bool Array3DDepthModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        _value->deleteDepth(row);
    }

    endRemoveRows();

    return false;
}

bool Array3DDepthModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array3DDepthModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

//===

Array3DModel::Array3DModel(const std::shared_ptr<Materials::MaterialProperty>& property,
                           const std::shared_ptr<Materials::Array3D>& value,
                           QObject* parent)
    : AbstractArrayModel(parent)
    , _property(property)
    , _value(value)
{}

int Array3DModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;  // No children
    }

    try {
        return _value->rows() + 1;  // Will always have 1 empty row
    }
    catch (const Materials::InvalidIndex&) {
        return 1;
    }
}

int Array3DModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return _property->columns() - 1;
}

bool Array3DModel::newRow(const QModelIndex& index) const
{
    try {
        return (index.row() == _value->rows());
    }
    catch (const Materials::InvalidIndex&) {
        return true;
    }
}

void Array3DModel::deleteRow(const QModelIndex& index)
{
    removeRows(index.row(), 1);
    Q_EMIT dataChanged(index, index);
}

QVariant Array3DModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        try {
            Base::Quantity qq = _value->getValue(index.row(), index.column());
            return QVariant::fromValue(qq);
        }
        catch (const Materials::InvalidIndex&) {
        }
        catch (const std::exception& e) {
            Base::Console().error("The error message is: %s\n", e.what());
        }

        try {
            Base::Quantity qq = Base::Quantity(0, _property->getColumnUnits(index.column() + 1).toStdString());
            qq.setFormat(Materials::MaterialValue::getQuantityFormat());
            return QVariant::fromValue(qq);
        }
        catch (const Materials::InvalidIndex&) {
        }
    }

    return {};
}

QVariant Array3DModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            const Materials::MaterialProperty& column = _property->getColumn(section + 1);
            return column.getDisplayName();
        }
        if (orientation == Qt::Vertical) {
            // Vertical header
            if (section == (rowCount() - 1)) {
                return QStringLiteral("*");
            }
            return {section + 1};
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

bool Array3DModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);

    if (_value->depth() == 0) {
        // Create the first row
        // _value->addDepth(Base::Quantity(0, _property->getColumnUnits(0)));
        return false;
    }

    if (index.row() == _value->rows()) {
        insertRows(index.row(), 1);
    }
    try {
        _value->setValue(index.row(), index.column(), value.value<Base::Quantity>());
    }
    catch (const Materials::InvalidIndex&) {
        Base::Console().error("Array3DModel::setData - InvalidIndex");
    }

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array3DModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array3DModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    int columns = columnCount();
    for (int i = 0; i < count; i++) {
        auto rowPtr = std::make_shared<QList<Base::Quantity>>();
        for (int j = 0; j < columns; j++) {
            rowPtr->push_back(_property->getColumnNull(j).value<Base::Quantity>());
        }

        _value->insertRow(row, rowPtr);
    }

    endInsertRows();

    return false;
}

bool Array3DModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        _value->deleteRow(row);
    }

    endRemoveRows();

    return false;
}

bool Array3DModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array3DModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

void Array3DModel::updateData()
{
    beginResetModel();

    // The table has changed at this point, typically by setting the current depth.
    // The existing structure needs to be cleared and the table redrawn.

    endResetModel();
}
