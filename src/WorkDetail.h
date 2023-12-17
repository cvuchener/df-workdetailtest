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
#include "df/types.h"

class Unit;
class DwarfFortressData;

namespace DFHack { class Client; }
namespace dfproto::workdetailtest {
class WorkDetailId;
class WorkDetailResult;
}

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

	WorkDetail(std::unique_ptr<df::work_detail> &&work_detail, DwarfFortressData &df, DFHack::Client &dfhack, QObject *parent = nullptr);
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
		std::vector<std::pair<df::unit_labor_t, bool>> labors;
		static decltype(labors) fromLabors(std::span<const bool, df::unit_labor::Count> labors);
	};
	QCoro::Task<> assign(int unit_id, bool assign);
	QCoro::Task<> assign(std::vector<int> units, bool assign);
	QCoro::Task<> toggle(std::vector<int> units);
	QCoro::Task<> edit(Properties properties);
	QCoro::Task<> remove();

	static QCoro::Task<> makeNewWorkDetail(std::shared_ptr<DwarfFortressData> df, QPointer<DFHack::Client> dfhack, Properties properties);

signals:
	void unitDataChanged(int unit_id);
	void aboutToBeUpdated();
	void updated();

private:
	void refresh();
	template<typename F>
	QCoro::Task<> changeAssignments(std::vector<int> units, F assign);
	void setAssignment(int unit_id, bool assign, ChangeStatus status);
	bool setId(dfproto::workdetailtest::WorkDetailId &id) const;
	void setProperties(const Properties &properties, const dfproto::workdetailtest::WorkDetailResult &results);

	std::unique_ptr<df::work_detail> _wd;
	DwarfFortressData &_df;
	QPointer<DFHack::Client> _dfhack;
	std::map<int, ChangeStatus> _statuses;

	QString _display_name;
};

#endif
