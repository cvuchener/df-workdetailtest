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

#ifndef LABOR_MODEL_H
#define LABOR_MODEL_H

#include <QAbstractItemModel>
#include "df_enums.h"

class LaborModel: public QAbstractItemModel
{
	Q_OBJECT
	Q_PROPERTY(bool group_by_category READ groupByCategory WRITE setGroupByCategory)
public:
	LaborModel(QObject *parent = nullptr);
	~LaborModel() override;

	QModelIndex index(int row, int column = 0, const QModelIndex &parent = {}) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	bool groupByCategory() const noexcept { return _group_by_category; }
	void setGroupByCategory(bool enabled);

	std::span<const bool, df::unit_labor::Count> labors() const { return _labors; }
	void setLabors(std::span<const bool, df::unit_labor::Count> labors);

private:
	std::variant<df::unit_labor_t, df::unit_labor_category_t> parseIndex(const QModelIndex &index) const;
	QVariant data(df::unit_labor_t labor, int role) const;
	QVariant data(df::unit_labor_category_t category, int role) const;
	bool setData(const QModelIndex &index, df::unit_labor_t labor, bool enable);
	bool setData(const QModelIndex &index, df::unit_labor_category_t category, bool enable);

	bool _group_by_category;
	std::array<bool, df::unit_labor::Count> _labors;
	static inline constexpr std::size_t CategoryCount = 13;
	std::array<std::vector<df::unit_labor_t>, CategoryCount> _categories;
};

#endif
