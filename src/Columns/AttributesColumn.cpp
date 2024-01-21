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

#include "AttributesColumn.h"

#include "Unit.h"
#include "UnitDescriptors.h"
#include "DataRole.h"

using namespace Columns;

QString attribute_title(const Unit::attribute_t &attr)
{
	return visit([](auto attr) { return UnitDescriptors::attributeName(attr); }, attr);
}

AttributesColumn::AttributesColumn(std::span<const Unit::attribute_t> attrs, QObject *parent):
	AbstractColumn(parent)
{
	_attrs.resize(attrs.size());
	std::ranges::copy(attrs, _attrs.begin());
}

AttributesColumn::~AttributesColumn()
{
}

int AttributesColumn::count() const
{
	return _attrs.size();
}

QVariant AttributesColumn::headerData(int section, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return attribute_title(_attrs.at(section));
	default:
		return {};
	}
}

template <typename T>
struct is_physical;
template <>
struct is_physical<df::physical_attribute_type_t>: std::true_type {};
template <>
struct is_physical<df::mental_attribute_type_t>: std::false_type {};
template <typename T>
static constexpr bool is_physical_v = is_physical<T>::value;

QVariant AttributesColumn::unitData(int section, const Unit &unit, int role) const
{
	const auto &attr = _attrs.at(section);
	auto unit_attr = unit.attribute(attr);
	if (!unit_attr)
		return {};
	switch (role) {
	case Qt::DisplayRole:
		return unit.attributeCasteRating(attr);
	case DataRole::RatingRole:
		return unit.attributeCasteRating(attr)/100.0;
	case DataRole::SortRole:
		return unit.attributeValue(attr);
	case Qt::ToolTipRole: {
		auto tooltip = tr("<h3>%1 - %2</h3>")
			.arg(unit.displayName())
			.arg(attribute_title(attr));
		tooltip += QString("<p>%1/%2</p>")
			.arg(unit.attributeValue(attr))
			.arg(unit_attr->max_value);
		visit([&](auto attr) {
			static constexpr const char *description = is_physical_v<decltype(attr)>
				? QT_TR_NOOP("%1 is %2") // physical
				: QT_TR_NOOP("%1 has %2"); // mental
			auto attr_rating = unit.attributeCasteRating(attr);
			auto attr_text = UnitDescriptors::attributeDescription(attr, attr_rating);
			if (!attr_text.isEmpty())
				tooltip += "<p>"+tr(description)
					.arg(unit.displayName())
					.arg(attr_text)+"</p>";
		}, attr);
		return tooltip;
	}
	default:
		return {};
	}
}

#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(GridViewLog);

Factory AttributesColumn::makeFactory(const QJsonObject &json)
{
	std::vector<Unit::attribute_t> attrs;
	for (auto value: json["attributes"].toArray()) {
		auto attr_name = value.toString().toLocal8Bit();
		std::string_view attr_view(attr_name.data(), attr_name.size());
		if (auto phys = df::physical_attribute_type::from_string(attr_view))
			attrs.push_back(*phys);
		else if (auto ment = df::mental_attribute_type::from_string(attr_view))
			attrs.push_back(*ment);
		else
			qCWarning(GridViewLog) << "Invalid attribute value for Attributes column" << attr_name;
	}
	return [attrs = std::move(attrs)](DwarfFortressData &df) {
		return std::make_unique<AttributesColumn>(attrs);
	};
}
