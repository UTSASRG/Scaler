package com.xttechgroup.scaler.analyzerserv.controller.restful;


import com.xttechgroup.scaler.analyzerserv.models.POJO.MultipleElfIds;
import com.xttechgroup.scaler.analyzerserv.models.repository.ProfilingInfoRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
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

    @PostMapping("/totalTime")
    Long getThreadTotalTime(Long jobid, @RequestBody MultipleElfIds body,
                            HttpServletRequest request,
                            HttpServletResponse response) {
        return profilingInfoRepo.getThreadTotalTime(jobid, body.getVisibleProcesses(), body.getVisibleThreads());
    }
}
