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

#include "Dock.h"

#include <QSortFilterProxyModel>

#include "ui_PreferencesDock.h"

#include "DwarfFortressData.h"
#include "Unit.h"
#include "MainWindow.h"
#include "UserUnitFilters.h"

#include "Model.h"

using namespace Preferences;

Dock::Dock(std::shared_ptr<const DwarfFortressData> df, QWidget *parent):
	QDockWidget(parent),
	_ui(std::make_unique<Ui::PreferencesDock>()),
	_df(std::move(df)),
	_model(std::make_unique<Model>(_df))
{
	_ui->setupUi(this);
	auto sort_model = new QSortFilterProxyModel(this);
	sort_model->setSourceModel(_model.get());
	_ui->preferences->setModel(sort_model);

	connect(_ui->preferences, &QAbstractItemView::activated,
		this, [this, sort_model](const auto &index) {
			if (auto main_window = qobject_cast<MainWindow *>(parentWidget())) {
				if (auto filters = main_window->currentFilters()) {
					setUnitPreferenceFilter(sort_model->mapToSource(index), *filters);
				}
			}
		});
}

Dock::~Dock()
{
}

void Dock::setUnitPreferenceFilter(const QModelIndex &index, UserUnitFilters &filters)
{
	filters.setAutoFilter(UserUnitFilters::AutoFilterID::Preferences,
		[pref = _model->get(index)](const Unit &unit) {
			if (!unit->current_soul)
				return false;
			return std::ranges::any_of(unit->current_soul->preferences,
					[&](const auto &p) { return *p == pref; });
		});
}
