package com.xttechgroup.scaler.analyzerserv.controller.restful;


import com.xttechgroup.scaler.analyzerserv.models.repository.ProfilingInfoRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Collection;

@RestController
@RequestMapping("/profiling")
public class ProfilingInfoRestController {
    @Autowired
    ProfilingInfoRepo profilingInfoRepo;

    @GetMapping("/threads")
    Collection<Long> getThreadIds(Long jobid) {
        return profilingInfoRepo.getThreadIds(jobid);
    }

    @GetMapping("/processes")
    Collection<Long> getProcessIds(Long jobid) {
        return profilingInfoRepo.getProcessIds(jobid);
    }
}
