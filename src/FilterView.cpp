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

#include "FilterView.h"

#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QToolButton>

struct FilterView::Ui
{
	QHBoxLayout layout;

	Ui(FilterView *parent):
		layout(parent)
	{
	}
};

FilterView::FilterView(QWidget *parent):
	QWidget(parent),
	_model(nullptr),
	_ui(std::make_unique<Ui>(this))
{
}

FilterView::~FilterView()
{
}

void FilterView::setModel(QAbstractItemModel *model)
{
	if (_model)
		disconnect(_model);
	while (auto item = _ui->layout.takeAt(0)) {
		delete item->widget();
		delete item;
	}
	_model = model;
	if (!_model)
		return;
	connect(_model, &QAbstractItemModel::rowsInserted, this, &FilterView::addItems);
	connect(_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &FilterView::removeItems);
	for (int i = 0; i < _model->rowCount(); ++i)
		addButton(i);
}

void FilterView::addItems(const QModelIndex &parent, int first, int last)
{
	if (parent.isValid())
		return;
	for (int i = first; i <= last; ++i)
		addButton(i);
}

void FilterView::removeItems(const QModelIndex &parent, int first, int last)
{
	if (parent.isValid())
		return;
	for (int i = last; i >= first; --i) {
		auto item = _ui->layout.takeAt(i);
		delete item->widget();
		delete item;
	}
}

void FilterView::addButton(int i)
{
	Q_ASSERT(_model);
	auto index = _model->index(i, 0);
	auto data = _model->data(index, Qt::DisplayRole);
	auto button = new QToolButton(this);
	auto action = new QAction(data.toString(), button);
	action->setIcon(QIcon::fromTheme("edit-delete"));
	button->setDefaultAction(action);
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	connect(action, &QAction::triggered, this, [this, index = QPersistentModelIndex(index)]() {
		_model->removeRow(index.row());
	});
	_ui->layout.insertWidget(i, button);
}
