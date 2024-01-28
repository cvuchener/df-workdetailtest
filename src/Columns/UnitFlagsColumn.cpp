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

#include "UnitFlagsColumn.h"

#include "DataRole.h"
#include "DwarfFortressData.h"

#include <QVariant>

using namespace Columns;

UnitFlagsColumn::UnitFlagsColumn(std::span<const Unit::Flag> flags, DwarfFortressData &df, QObject *parent):
	AbstractColumn(parent),
	_df(df)
{
	_flags.resize(flags.size());
	std::ranges::copy(flags, _flags.begin());
}

UnitFlagsColumn::~UnitFlagsColumn()
{
}

int UnitFlagsColumn::count() const
{
	return _flags.size();
}

QVariant UnitFlagsColumn::headerData(int section, int role) const
{
	if (role != Qt::DisplayRole)
		return {};
	return title(_flags.at(section));
}

QVariant UnitFlagsColumn::unitData(int section, const Unit &unit, int role) const
{
	switch (role) {
	case Qt::CheckStateRole:
	case DataRole::SortRole:
		return unit.hasFlag(_flags.at(section))
			? Qt::Checked
			: Qt::Unchecked;
	default:
		return {};
	}
}

QVariant UnitFlagsColumn::groupData(int section, GroupBy::Group group, std::span<const Unit *> units, int role) const
{
	auto flag = _flags.at(section);
	auto has_flag = [flag](const Unit *u) { return u->hasFlag(flag); };
	switch (role) {
	case Qt::DisplayRole:
		return int(std::ranges::count_if(units, has_flag));
	case Qt::CheckStateRole:
		return std::ranges::all_of(units, has_flag)
			? Qt::Checked
			: std::ranges::none_of(units, has_flag)
				? Qt::Unchecked
				: Qt::PartiallyChecked;
	case Qt::ToolTipRole: {
		auto tooltip = QString("<h2>%1</h2>").arg(group.name());
		auto count = std::ranges::count_if(units, has_flag);
		if (count > 0) {
			tooltip.append("<p>"+countText(flag, count)+"</p>");
			tooltip.append("<ul>");
			for (auto unit: units)
				if (has_flag(unit))
					tooltip.append(QString("<li>%1</li>")
							.arg(unit->displayName()));
			tooltip.append("</ul>");
		}
		else
			tooltip.append("<p>"+countText(flag, 0)+"</p>");
		return tooltip;
	}
	case DataRole::SortRole:
		return int(std::ranges::count_if(units, has_flag));
	default:
		return {};
	}
}

static Unit::Properties makeProperties(Unit::Flag flag, const QVariant &value)
{
	return {.flags = {{flag, value.value<Qt::CheckState>() == Qt::Checked}}};
}

bool UnitFlagsColumn::setUnitData(int section, Unit &unit, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	auto flag = _flags.at(section);
	if (!unit.canEdit(flag))
		return false;
	unit.edit(makeProperties(flag, value));
	return true;
}

bool UnitFlagsColumn::setGroupData(int section, std::span<Unit *> units, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	auto flag = _flags.at(section);
	if (units.size() == 1 && units.front()->canEdit(flag)) {
		units.front()->edit(makeProperties(flag, value));
	}
	else {
		std::vector<std::shared_ptr<Unit>> unitptrs;
		unitptrs.reserve(units.size());
		for (auto unit: units) {
			if (!unit->canEdit(flag))
				continue;
			unitptrs.push_back(unit->shared_from_this());
		}
		Unit::edit(_df.shared_from_this(), std::move(unitptrs), makeProperties(flag, value));
	}
	return true;
}

void UnitFlagsColumn::toggleUnits(int section, std::span<Unit *> units)
{
	auto flag = _flags.at(section);
	std::vector<std::shared_ptr<Unit>> unitptrs;
	unitptrs.reserve(units.size());
	for (auto unit: units) {
		if (!unit->canEdit(flag))
			continue;
		unitptrs.push_back(unit->shared_from_this());
	}
	Unit::toggle(_df.shared_from_this(), std::move(unitptrs), flag);
}

Qt::ItemFlags UnitFlagsColumn::unitFlags(int section, const Unit &unit) const
{
	if (unit.canEdit(_flags.at(section)))
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

Qt::ItemFlags UnitFlagsColumn::groupFlags(int section, std::span<const Unit *> units) const
{
	auto flag = _flags.at(section);
	if (std::ranges::any_of(units, [flag](const Unit *unit) { return unit->canEdit(flag); }))
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

QString UnitFlagsColumn::title(Unit::Flag flag)
{
	switch (flag) {
	case Unit::Flag::OnlyDoAssignedJobs:
		return tr("Specialist");
	case Unit::Flag::AvailableForAdoption:
		return tr("Adoption");
	case Unit::Flag::MarkedForSlaugter:
		return tr("Slaughter");
	case Unit::Flag::MarkedForGelding:
		return tr("Geld");
	}
	Q_UNREACHABLE();
}

QString UnitFlagsColumn::countText(Unit::Flag flag, int count)
{
	if (count == 0) {
		switch (flag) {
		case Unit::Flag::OnlyDoAssignedJobs:
			return tr("There is no specialist");
		case Unit::Flag::AvailableForAdoption:
			return tr("No one is available for adoption");
		case Unit::Flag::MarkedForSlaugter:
			return tr("No one is marked for slaughter");
		case Unit::Flag::MarkedForGelding:
			return tr("No one is marked for gelding");
		}
	}
	else {
		switch (flag) {
		case Unit::Flag::OnlyDoAssignedJobs:
			return tr("%1 specialists").arg(count);
		case Unit::Flag::AvailableForAdoption:
			return tr("%1 available for adoption").arg(count);
		case Unit::Flag::MarkedForSlaugter:
			return tr("%1 marked for slaughter").arg(count);
		case Unit::Flag::MarkedForGelding:
			return tr("%1 marked for gelding").arg(count);
		}
	}
	Q_UNREACHABLE();
}

#include "DwarfFortressData.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaEnum>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(GridViewLog);

Factory UnitFlagsColumn::makeFactory(const QJsonObject &json)
{
	std::vector<Unit::Flag> flags;
	for (auto value: json["flags"].toArray()) {
		bool ok;
		auto flag_name = value.toString().toLocal8Bit();
		auto flag = QMetaEnum::fromType<Unit::Flag>().keyToValue(flag_name, &ok);
		if (ok)
			flags.push_back(static_cast<Unit::Flag>(flag));
		else
			qCWarning(GridViewLog) << "Invalid flag value for UnitFlags column" << flag_name;
	}
	return [flags = std::move(flags)](DwarfFortressData &df) {
		return std::make_unique<UnitFlagsColumn>(flags, df);
	};
}
