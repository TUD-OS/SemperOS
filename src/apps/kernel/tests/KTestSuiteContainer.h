/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of SemperOS.
 *
 * SemperOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * SemperOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#ifdef KERNEL_TESTS

#include <base/log/Kernel.h>

#include "KTestSuite.h"

namespace kernel {

class KTestSuiteContainer {
public:
    explicit KTestSuiteContainer()
        : _suites() {
    }
    ~KTestSuiteContainer() {
        for(auto it = _suites.begin(); it != _suites.end(); ) {
            auto old = it++;
            delete &*old;
        }
    }

    void add(KTestSuite* suite) {
        _suites.append(suite);
    }
    int run() {
        size_t suites_succ = 0;
        size_t suites_failed = 0;
        size_t cases_succ = 0;
        size_t cases_failed = 0;
        for(auto &s : _suites) {
            KLOG(INFO, "\033[1mTestsuite \"" << s.get_name() << "\"...\033[0m");

            s.run();

            if(s.get_failed() == 0)
                suites_succ++;
            else
                suites_failed++;

            KLOG(INFO, "  " << (s.get_failed() == 0 ? "\033[0;32m" : "\033[0;31m") << s.get_succeeded()
                    << "\033[0m of " << (s.get_failed() + s.get_succeeded()) << " testcases successfull");
            cases_succ += s.get_succeeded();
            cases_failed += s.get_failed();
        }

        KLOG(INFO, "\n\033[1mAll tests done:\033[0m");
        KLOG(INFO,(suites_succ == _suites.length() ? "\033[0;32m" : "\033[0;31m")
                 << suites_succ << "\033[0m of " << _suites.length() << " testsuites successfull");
        KLOG(INFO, (cases_failed == 0 ? "\033[0;32m" : "\033[0;31m") << cases_succ << "\033[0m of "
                 << (cases_succ + cases_failed) << " testcases successfull");
        return ((suites_succ + suites_failed) << 16) | suites_succ;
    }

private:
    m3::SList<KTestSuite> _suites;
};

}

#endif
