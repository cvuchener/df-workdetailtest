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

#ifndef FILTER_BAR_H
#define FILTER_BAR_H

#include <QToolBar>
#include <QWidgetAction>

enum class BuiltinFilter {
	Workers
};

enum class FilterType {
	Simple,
	Regex,
	Script,
};

class LabelAction: public QWidgetAction
{
	Q_OBJECT
public:
	LabelAction(const QString &text, QObject *parent = nullptr);
	~LabelAction();

protected:
	QWidget *createWidget(QWidget *parent) override;
};

class FilterTypeAction: public QWidgetAction
{
	Q_OBJECT
public:
	FilterTypeAction(QObject *parent = nullptr);
	~FilterTypeAction();

	FilterType currentType() const { return _type; }

signals:
	void currentChanged(FilterType);

protected:
	QWidget *createWidget(QWidget *parent) override;

private:
	FilterType _type;
};

class FilterTextAction: public QWidgetAction
{
	Q_OBJECT
public:
	FilterTextAction(QObject *parent = nullptr);
	~FilterTextAction() override;

	const QString &text() const { return _text; }

signals:
	void textChanged(const QString &);

protected:
	QWidget *createWidget(QWidget *parent) override;

private:
	QString _text;
};

class RemoveFilterAction: public QWidgetAction
{
	Q_OBJECT
public:
	RemoveFilterAction(const QString &text, QObject *parent = nullptr);
	~RemoveFilterAction() override;

protected:
	QWidget *createWidget(QWidget *parent) override;
};

class FilterMenuAction: public QWidgetAction
{
	Q_OBJECT
public:
	FilterMenuAction(QObject *parent = nullptr);
	~FilterMenuAction();

signals:
	void triggered(const QAction *);

protected:
	QWidget *createWidget(QWidget *parent) override;
};

class UnitFilterList;

class FilterBar: public QToolBar
{
	Q_OBJECT
public:
	FilterBar(QWidget *parent);
	~FilterBar() override;

	void setFilterModel(UnitFilterList *model);

signals:
	void filterChanged(FilterType type, const QString &text);

private:
	void insertFilterButtons(int first, int last);
	void filterInserted(const QModelIndex &parent, int first, int last);
	void filterRemoved(const QModelIndex &parent, int first, int last);

	FilterTypeAction *_filter_type;
	FilterTextAction *_filter_text;
	UnitFilterList *_filters;
	std::vector<std::unique_ptr<QAction>> _remove_filter_actions;
	FilterMenuAction *_add_filter_menu;
};

#endif
