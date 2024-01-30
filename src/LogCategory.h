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

#ifndef LOG_CATEGORY_H
#define LOG_CATEGORY_H

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ScriptLog);
Q_DECLARE_LOGGING_CATEGORY(WorkDetailLog);
Q_DECLARE_LOGGING_CATEGORY(GridViewLog);
Q_DECLARE_LOGGING_CATEGORY(StructuresLog);
Q_DECLARE_LOGGING_CATEGORY(DFHackLog);
Q_DECLARE_LOGGING_CATEGORY(ProcessLog);

#endif
