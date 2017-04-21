/*
 * Copyright (c) 2017 Sven Willner <sven.willner@pik-potsdam.de>
 * Free software under GNU General Public License v3, see LICENSE
 */

#include "HectorWrapper.h"
#include "components/component_data.hpp"
#include "data/message_data.hpp"
#include "data/unitval.hpp"
#include "h_exception.hpp"

namespace Hector {
void OutputVisitor::add_variable(const std::string& component, const std::string& name, const bool need_date, const bool in_spinup) {
    start_date = wrapper_->hcore()->getStartDate();
    Hector::IModelComponent* component_;
    if (component == "core") {
        component_ = nullptr;
    } else {
        component_ = wrapper_->hcore()->getComponentByName(component);
    }
    if (in_spinup) {
        variables.emplace_back(OutputVariable{component_, name, std::vector<double>(), need_date, in_spinup});
    } else {
        variables.emplace_back(
            OutputVariable{component_, name, std::vector<double>(static_cast<int>(wrapper_->hcore()->getEndDate() - start_date + 1)), need_date, in_spinup});
    }
}

const std::vector<double>& OutputVisitor::get_variable(const std::string& component, const std::string& name, const bool in_spinup) const {
    for (auto& variable : variables) {
        if (in_spinup == variable.in_spinup && name == variable.name && component == variable.component->getComponentName()) {
            return variable.values;
        }
    }
    throw hector_wrapper_exception("Variable not found");
}

bool OutputVisitor::shouldVisit(const bool in_spinup, const double date) {
    current_date = date;
    return true;
}

void OutputVisitor::visit(Hector::Core* core) {
    const unsigned int index = static_cast<int>(current_date - start_date - 1);
    const bool in_spinup = core->inSpinup();
    if (in_spinup) {
        spinup_size_++;
    }
    for (auto& variable : variables) {
        if (variable.in_spinup == in_spinup) {
            Hector::message_data info;
            if (variable.needs_date) {
                info.date = current_date;
            }
            if (variable.component) {
                if (in_spinup) {
                    variable.values.push_back(variable.component->sendMessage(M_GETDATA, variable.name, info));
                } else {
                    variable.values[index] = variable.component->sendMessage(M_GETDATA, variable.name, info);
                }
            } else {
                if (in_spinup) {
                    variable.values.push_back(core->sendMessage(M_GETDATA, variable.name, info));
                } else {
                    variable.values[index] = core->sendMessage(M_GETDATA, variable.name, info);
                }
            }
        }
    }
};

int OutputVisitor::run_size() const { return static_cast<int>(wrapper_->hcore()->getEndDate() - start_date); }

int OutputVisitor::spinup_size() const { return spinup_size_; }

void OutputVisitor::reset() { variables.clear(); }

void OutputVisitor::prepare_to_run() { spinup_size_ = 0; }

HectorWrapper::HectorWrapper() : output_visitor(this) {
    Hector::Logger& glog = Hector::Logger::getGlobalLogger();
    glog.close();
    glog.open("hector", false, Hector::Logger::LogLevel::WARNING, false);
    hcore_.init();
    hcore_.addVisitor(&output_visitor);
}

void HectorWrapper::run() {
    output_visitor.prepare_to_run();
    hcore_.prepareToRun();
    hcore_.run();
}

void HectorWrapper::reset() { output_visitor.reset(); }

void HectorWrapper::set(const std::string& section, const std::string& variable, const std::string& value) {
    message_data data(value);
    auto bracket_open = std::find(variable.begin(), variable.end(), '[');
    if (bracket_open != variable.end()) {
        data.date = std::stod(std::string(bracket_open + 1, std::find(bracket_open, variable.end(), ']')));
        hcore_.setData(section, std::string(variable.begin(), bracket_open), data);
    } else {
        message_data data(value);
        hcore_.setData(section, variable, data);
    }
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const double value) {
    message_data data(unitval(value, U_UNDEFINED));
    hcore_.setData(section, variable, data);
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const int year, const double value) {
    message_data data(unitval(value, U_UNDEFINED));
    data.date = year;
    hcore_.setData(section, variable, data);
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const int* years, const double* values, const size_t size) {
    for (unsigned int i = 0; i < size; ++i) {
        message_data data(unitval(values[i], U_UNDEFINED));
        data.date = years[i];
        hcore_.setData(section, variable, data);
    }
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const std::vector<int>& years, const std::vector<double>& values) {
    if (years.size() != values.size()) {
        throw hector_wrapper_exception("years and values should be of equal size");
    }
    set(section, variable, &years[0], &values[0], years.size());
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const double value, const std::string& unit) {
    message_data data(unitval(value, unitval::parseUnitsName(unit)));
    hcore_.setData(section, variable, data);
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const int year, const double value, const std::string& unit) {
    message_data data(unitval(value, unitval::parseUnitsName(unit)));
    data.date = year;
    hcore_.setData(section, variable, data);
}

void HectorWrapper::set(
    const std::string& section, const std::string& variable, const int* years, const double* values, const size_t size, const std::string& unit) {
    for (unsigned int i = 0; i < size; ++i) {
        message_data data(unitval(values[i], unitval::parseUnitsName(unit)));
        data.date = years[i];
        hcore_.setData(section, variable, data);
    }
}

void HectorWrapper::set(
    const std::string& section, const std::string& variable, const std::vector<int>& years, const std::vector<double>& values, const std::string& unit) {
    if (years.size() != values.size()) {
        throw hector_wrapper_exception("years and values should be of equal size");
    }
    set(section, variable, &years[0], &values[0], years.size(), unit);
}
}
