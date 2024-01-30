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

#include "Dock.h"

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QHeaderView>

#include "ui_UnitDetailsDock.h"

#include "DwarfFortressData.h"
#include "Unit.h"

#include "DataRole.h"

#include "ProgressDelegate.h"

#include "AttributeModel.h"
#include "InventoryModel.h"
#include "SkillModel.h"

using namespace UnitDetails;

static const char *NamePlaceholder = QT_TRANSLATE_NOOP(Dock, "Select a unit");

Dock::Dock(std::shared_ptr<const DwarfFortressData> df, QWidget *parent):
	QDockWidget(tr("Unit details"), parent),
	_ui(std::make_unique<Ui::UnitDetailsDock>()),
	_df(std::move(df))
{
	_ui->setupUi(this);
	_ui->unit_name->setText(tr(NamePlaceholder));
	_ui->unit_profession->setText({});
	_ui->unit_age->setText({});

	auto make_view = [this](const QString &title, UnitDataModel *model) {
		auto sort_model = new QSortFilterProxyModel(this);
		sort_model->setSourceModel(model);
		sort_model->setSortRole(DataRole::SortRole);
		auto view = new QTreeView(this);
		view->setSortingEnabled(true);
		view->setRootIsDecorated(false);
		view->setModel(sort_model);
		_ui->tabs->addTab(view, title);
		_models.push_back(model);
		_views.push_back(view);
		return view;
	};

	auto progress_delegate = new ProgressDelegate(this);

	auto skill_model = new SkillModel(*_df, this);
	[[maybe_unused]] auto skill_view = make_view(tr("Skills"), skill_model);
	skill_view->sortByColumn(static_cast<int>(SkillModel::Column::Level), Qt::DescendingOrder);
	skill_view->setItemDelegateForColumn(static_cast<int>(SkillModel::Column::Progress), progress_delegate);

	auto attr_model = new AttributeModel(*_df, this);
	[[maybe_unused]] auto attr_view = make_view(tr("Attributes"), attr_model);

	auto inventory_model = new InventoryModel(*_df, this);
	[[maybe_unused]] auto inventory_view = make_view(tr("Inventory"), inventory_model);
}

Dock::~Dock()
{
}

void Dock::setUnit(const Unit *unit)
{
	if (_current_unit_destroyed)
		disconnect(_current_unit_destroyed);
	if (unit) {
		_current_unit_destroyed = connect(unit, &QObject::destroyed,
				this, [this](){setUnit(nullptr);});
		_ui->unit_name->setText(unit->displayName());
		_ui->unit_profession->setText(QString::fromLocal8Bit(caption((*unit)->profession)));
		auto age = df::date<df::year, df::month>(unit->age());
		if (get<df::year>(age).count() > 0) {
			auto years = get<df::year>(age).count();
			_ui->unit_age->setText(tr("%1 year(s) old", nullptr, years).arg(years));
		}
		else {
			auto months = get<df::month>(age).count();
			_ui->unit_age->setText(tr("%1 month(s) old", nullptr, months).arg(months));
		}

	}
	else {
		_ui->unit_name->setText(NamePlaceholder);
		_ui->unit_profession->setText({});
		_ui->unit_age->setText({});
	}
	for (auto model: _models)
		model->setUnit(unit);
	for (auto view: _views)
		view->header()->resizeSections(QHeaderView::ResizeToContents);
}

