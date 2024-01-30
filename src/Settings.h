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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

#include <QIntValidator>
#include <QDoubleValidator>
#include <QRegularExpressionValidator>

#include "StandardPaths.h"

class SettingPropertyBase: public QObject
{
	Q_OBJECT
public:
	SettingPropertyBase(QObject *parent = nullptr);
	~SettingPropertyBase() override;

signals:
	void valueChanged();
};

template <typename T>
class SettingProperty: public SettingPropertyBase
{
public:
	using validator_t =
			std::conditional_t<std::integral<T>, QIntValidator,
			std::conditional_t<std::floating_point<T>, QDoubleValidator,
			std::conditional_t<std::is_same_v<T, QString>, QRegularExpressionValidator,
			std::monostate>>>;
	template <typename... ValidatorArgs> requires std::constructible_from<validator_t, ValidatorArgs...>
	SettingProperty(const QString &name, T default_value, ValidatorArgs &&...validator_args):
		SettingPropertyBase(nullptr),
		_name(name),
		_default_value(default_value),
		_value(StandardPaths::settings().value(name, QVariant::fromValue(default_value)).template value<T>()),
		_validator(std::forward<ValidatorArgs>(validator_args)...)
	{
	}
	~SettingProperty() override = default;

	// getters
	const T &value() const noexcept { return _value; }
	const T &operator()() const noexcept { return _value; }

	const T &defaultValue() const noexcept { return _default_value; }

	const validator_t *validator() const noexcept { return &_validator; }

	// setters
	template <typename U> requires std::assignable_from<T &, U &&>
	void setValue(U &&value) {
		if (value != _value) {
			_value = std::forward<U>(value);
			StandardPaths::settings().setValue(_name, QVariant::fromValue(_value));
			valueChanged();
		}
	}
	template <typename U> requires std::assignable_from<T &, U &&>
	SettingProperty &operator=(U &&value) {
		setValue(std::forward<U>(value));
		return *this;
	}

private:
	const QString _name;
	const T _default_value;
	T _value;
	validator_t _validator;
};

enum class RatingDisplay
{
	Text,
	GrowingBox,
};

struct Settings
{
	SettingProperty<QString> host_address = {"host/address", "localhost"};
	SettingProperty<quint16> host_port = {"host/port", 5000, 1, 65534};
	SettingProperty<bool> autoconnect = {"host/connect_on_startup", false};

	SettingProperty<bool> autorefresh_enabled = {"autorefresh/enabled", true};
	SettingProperty<double> autorefresh_interval = {"autorefresh/interval", 2.0};

	SettingProperty<bool> use_native_process = {"process/use_native", true};

	SettingProperty<bool> per_view_group_by = {"gridview/per_view_group_by", false};
	SettingProperty<bool> per_view_filters = {"gridview/per_view_filter", false};

	SettingProperty<bool> bypass_work_detail_protection = {"work_details/bypass_protection", false};

	SettingProperty<RatingDisplay> rating_display_mode = {"gridview/display_rating", RatingDisplay::GrowingBox};
};

#endif
