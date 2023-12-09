/*
 * Copyright 2023 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "NameColumn.h"

#include "df/utils.h"
#include "Unit.h"
#include "DataRole.h"

#include <QVariant>

NameColumn::NameColumn(QObject *parent):
	AbstractColumn(parent),
	_sort{*this, SortBy::Name, {
		{SortBy::Name, tr("name")},
		{SortBy::Age, tr("age")}}}
{
}

NameColumn::~NameColumn()
{
}

QVariant NameColumn::headerData(int section, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return tr("Name");
	default:
		return {};
	}
}

QVariant NameColumn::unitData(int section, const Unit &unit, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return unit.displayName();
	case Qt::EditRole:
		return df::fromCP437(unit->name.nickname);
	case DataRole::SortRole:
		switch (_sort.option) {
		case SortBy::Name:
			return unit.displayName();
		case SortBy::Age:
			return qlonglong(unit.age().count());
		}
	default:
		return {};
	}
}

QVariant NameColumn::groupData(int section, GroupBy::Group group, std::span<const Unit *> units, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return group.name();
	case DataRole::SortRole:
		return group.sortValue();
	default:
		return {};
	}
}

bool NameColumn::setUnitData(int section, Unit &unit, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;
	unit.edit({ .nickname = value.toString() });
	return true;
}

Qt::ItemFlags NameColumn::unitFlags(int section, const Unit &unit) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

Qt::ItemFlags NameColumn::groupFlags(int section, std::span<const Unit *> units) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

#include <QMenu>
#include <QInputDialog>

void NameColumn::makeHeaderMenu(int section, QMenu *menu, QWidget *parent)
{
	_sort.makeSortMenu(menu);
}

void NameColumn::makeUnitMenu(int section, Unit &unit, QMenu *menu, QWidget *parent)
{
	auto nickname_action = new QAction(tr("Edit %1 nickname...").arg(unit.displayName()), menu);
	connect(nickname_action, &QAction::triggered, [&unit, parent]() {
		bool ok;
		auto new_nickname = QInputDialog::getText(parent,
				tr("Edit nickname"),
				tr("Choose a new nickname for %1:").arg(unit.displayName()),
				QLineEdit::Normal,
				df::fromCP437(unit->name.nickname),
				&ok);
		if (ok)
			unit.edit({ .nickname = new_nickname });
	});
	menu->addAction(nickname_action);
}
