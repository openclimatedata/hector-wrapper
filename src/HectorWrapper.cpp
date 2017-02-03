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
void OutputVisitor::add_variable(const std::string& component, const std::string& name, const bool need_date) {
    start_date = wrapper_->hcore()->getStartDate();
    Hector::IModelComponent* component_;
    if (component == "core") {
        component_ = nullptr;
    } else {
        component_ = wrapper_->hcore()->getComponentByName(component);
    }
    OutputVariable variable = {component_, name, std::vector<double>(static_cast<int>(wrapper_->hcore()->getEndDate() - start_date + 1)), need_date};
    variables.emplace_back(variable);
}

const std::vector<double>& OutputVisitor::get_variable(const std::string& component, const std::string& name) const {
    for (auto& variable : variables) {
        if (name == variable.name && component == variable.component->getComponentName()) {
            return variable.values;
        }
    }
    throw hector_wrapper_exception("Variable not found");
}

bool OutputVisitor::shouldVisit(const bool in_spinup, const double date) {
    current_date = date;
    return !in_spinup;
}

void OutputVisitor::visit(Hector::Core* core) {
    unsigned int index = static_cast<int>(current_date - start_date - 1);
    for (auto& variable : variables) {
        Hector::message_data info;
        if (variable.needs_date) {
            info.date = current_date;
        }
        if (variable.component) {
            variable.values[index] = variable.component->sendMessage(M_GETDATA, variable.name, info);
        } else {
            variable.values[index] = core->sendMessage(M_GETDATA, variable.name, info);
        }
    }
};

int OutputVisitor::run_size() { return static_cast<int>(wrapper_->hcore()->getEndDate() - start_date); }

HectorWrapper::HectorWrapper() : output_visitor(this) {
    Hector::Logger& glog = Hector::Logger::getGlobalLogger();
    glog.close();
    glog.open("hector", false, Hector::Logger::LogLevel::WARNING, false);
    hcore_.init();
    hcore_.addVisitor(&output_visitor);
}

HectorWrapper::~HectorWrapper(){};

void HectorWrapper::run() {
    hcore_.prepareToRun();
    hcore_.run();
}

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

void HectorWrapper::set(const std::string& section, const std::string& variable, const int* years, const double* values, const size_t size, const std::string& unit) {
    for (unsigned int i = 0; i < size; ++i) {
        message_data data(unitval(values[i], unitval::parseUnitsName(unit)));
        data.date = years[i];
        hcore_.setData(section, variable, data);
    }
}

void HectorWrapper::set(const std::string& section, const std::string& variable, const std::vector<int>& years, const std::vector<double>& values, const std::string& unit) {
    if (years.size() != values.size()) {
        throw hector_wrapper_exception("years and values should be of equal size");
    }
    set(section, variable, &years[0], &values[0], years.size(), unit);
}
}
