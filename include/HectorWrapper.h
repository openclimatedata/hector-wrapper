/*
 * Copyright (c) 2017 Sven Willner <sven.willner@pik-potsdam.de>
 * Free software under GNU General Public License v3, see LICENSE
 */

#ifndef HECTOR_WRAPPER_H
#define HECTOR_WRAPPER_H

#include <stdexcept>
#include <string>
#include <vector>
#include "components/imodel_component.hpp"
#include "core/core.hpp"
#include "core/logger.hpp"
#include "visitors/avisitor.hpp"

namespace Hector {

class HectorWrapper;

class hector_wrapper_exception : public std::runtime_error {
  public:
    hector_wrapper_exception(const std::string& s) : std::runtime_error(s) {}
};

class OutputVisitor : public AVisitor {
  private:
    double start_date;
    double current_date;
    double end_date;
    const HectorWrapper* wrapper_;
    struct OutputVariable {
        IModelComponent* component;
        std::string name;
        std::vector<double> values;
        bool needs_date;
    };
    std::vector<OutputVariable> variables;

  public:
    OutputVisitor(const HectorWrapper* wrapper_p) : wrapper_(wrapper_p) {};
    void add_variable(const std::string& component, const std::string& name, const bool need_date = false);
    const std::vector<double>& get_variable(const std::string& component, const std::string& name) const;
    bool shouldVisit(const bool in_spinup, const double date);
    void visit(Core* core);
    int run_size();
};

class HectorWrapper {
  protected:
    Core hcore_;
    OutputVisitor output_visitor;

  public:
    HectorWrapper();
    virtual ~HectorWrapper();

    inline OutputVisitor* output() { return &output_visitor; }
    inline const OutputVisitor* output() const { return &output_visitor; }
    inline Core* hcore() { return &hcore_; }
    inline const Core* hcore() const { return &hcore_; }
    virtual void run();
    void set(const std::string& section, const std::string& variable, const std::string& value);
    void set(const std::string& section, const std::string& variable, const double value);
    void set(const std::string& section, const std::string& variable, const int year, const double value);
    void set(const std::string& section, const std::string& variable, const int* years, const double* values, const size_t size);
    void set(const std::string& section, const std::string& variable, const std::vector<int>& years, const std::vector<double>& values);
    void set(const std::string& section, const std::string& variable, const double value, const std::string& unit);
    void set(const std::string& section, const std::string& variable, const int year, const double value, const std::string& unit);
    void set(const std::string& section, const std::string& variable, const int* years, const double* values, const size_t size, const std::string& unit);
    void set(const std::string& section, const std::string& variable, const std::vector<int>& years, const std::vector<double>& values, const std::string& unit);
};
}

#endif
