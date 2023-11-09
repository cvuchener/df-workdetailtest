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

#ifndef WORKDETAIL_H
#define WORKDETAIL_H

#include <QObject>
#include <QCoroTask>
#include "DFTypes.h"

class Unit;
class DwarfFortress;

class WorkDetail: public QObject, public std::enable_shared_from_this<WorkDetail>
{
	Q_OBJECT
public:
	enum ChangeStatus
	{
		NoChange,
		Pending,
		Failed,
	};
	Q_ENUM(ChangeStatus)

	WorkDetail(std::unique_ptr<df::work_detail> &&work_detail, DwarfFortress &df, QObject *parent = nullptr);
	~WorkDetail() override;

	using df_type = df::work_detail;
	static inline constexpr auto name_member = &df::work_detail::name;

	void update(std::unique_ptr<df::work_detail> &&work_detail);

	const df::work_detail *get() const { return _wd.get(); }
	const df::work_detail &operator*() const { return *_wd; }
	const df::work_detail *operator->() const { return _wd.get(); }

	const QString &displayName() const { return _display_name; }
	ChangeStatus status(int unit_id) const  {
		auto it = _statuses.find(unit_id);
		if (it == _statuses.end())
			return NoChange;
		else
			return it->second;
	}

	bool isAssigned(int unit_id) const;

	struct Properties {
		QString name;
		std::optional<df::work_detail_mode_t> mode;
		std::optional<df::work_detail_icon_t> icon;
	};
	QCoro::Task<> assign(int unit_id, bool assign);
	QCoro::Task<> edit(Properties properties);

signals:
	void unitDataChanged(int unit_id);
	void aboutToBeUpdated();
	void updated();

private:
	void refresh();

	std::unique_ptr<df::work_detail> _wd;
	DwarfFortress &_df;
	std::map<int, ChangeStatus> _statuses;

	QString _display_name;
};

#endif
