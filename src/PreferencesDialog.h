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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>

namespace Ui { class PreferencesDialog; }

class PreferencesDialog: public QDialog
{
	Q_OBJECT
public:
	PreferencesDialog(QWidget *parent = nullptr);
	~PreferencesDialog() override;

public:
	void loadSettings();
	void loadDefaultSettings();
	void saveSettings() const;

private:
	std::unique_ptr<Ui::PreferencesDialog> _ui;
};

#endif
