/*
 * Copyright 2024 Clement Vuchener
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

#ifndef PREFERENCES_MODEL_H
#define PREFERENCES_MODEL_H

#include <QAbstractTableModel>

class DwarfFortressData;

namespace Ui { class PreferencesDock; }

namespace df { struct unit_preference; }

namespace Preferences
{

class Model: public QAbstractTableModel
{
	Q_OBJECT
public:
	Model(std::shared_ptr<const DwarfFortressData> df, QObject *parent = nullptr);
	~Model() override;

	enum class Columns {
		Type,
		Name,
		UnitCount,
		Count // Column count
	};

	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
	std::shared_ptr<const DwarfFortressData> _df;
	std::vector<std::pair<df::unit_preference, int>> _preferences;

	void rebuild();
};

} // namespace Preferences

#endif

