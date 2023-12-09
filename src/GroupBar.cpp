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

#include "GroupBar.h"

#include <QWidgetAction>
#include <QComboBox>
#include <QLabel>
#include <QCoreApplication>

#include "Groups/Factory.h"

struct GroupBar::Ui
{
	QWidgetAction *group_by_action;
	QComboBox *group_by_cb;

	void setupUi(QToolBar *parent)
	{
		parent->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		auto title_action = new QWidgetAction(parent);
		title_action->setDefaultWidget(new QLabel(tr("Group by: ")));
		parent->addAction(title_action);

		group_by_action = new QWidgetAction(parent);
		group_by_cb = new QComboBox;
		for (const auto &[name, factory]: Groups::All)
			group_by_cb->addItem(QCoreApplication::translate("Groups", name));
		group_by_action->setDefaultWidget(group_by_cb);
		parent->addAction(group_by_action);
	}
};

GroupBar::GroupBar(QWidget *parent):
	QToolBar(tr("Groups"), parent),
	_ui(std::make_unique<GroupBar::Ui>())
{
	_ui->setupUi(this);
	connect(_ui->group_by_cb, &QComboBox::currentIndexChanged, this, [this](int index) {
		groupChanged(index);
	});
}

GroupBar::~GroupBar()
{
}

void GroupBar::setGroup(int index)
{
	_ui->group_by_cb->setCurrentIndex(index);
}
