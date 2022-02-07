package com.xttechgroup.scaler.analyzerserv.controller.restful;

import com.xttechgroup.scaler.analyzerserv.models.POJO.ElfImgInfoQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.POJO.ExecutionCommand;
import com.xttechgroup.scaler.analyzerserv.models.POJO.MultipleElfIds;
import com.xttechgroup.scaler.analyzerserv.models.POJO.UpdateConfigReqBody;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.repository.ConfigRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/config")
public class ConfigRestController {
    @Autowired
    ConfigRepo configRepo;
    @Autowired
    Map<Long, ExecutionCommand> configRequestMap;

    @GetMapping("/yamlconfig")
    public String getYamlConfig(Long jobid, HttpServletRequest request,
                                HttpServletResponse response) {
        if (jobid == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }
        return configRepo.getConfigYaml(jobid);
    }

    @PostMapping("/yamlconfig")
    public String setYamlConfig(Long jobid, @RequestBody UpdateConfigReqBody body, HttpServletRequest request,
                                HttpServletResponse response) {
        if (jobid == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }
        configRepo.setConfigYaml(jobid, body.getYamlConfig());
        ExecutionCommand curMonitor = configRequestMap.get(jobid);
        if (curMonitor == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return null;
        } else {
            synchronized (curMonitor) {
                curMonitor.execute = true;
                curMonitor.yamlConfig=body.getYamlConfig();
                curMonitor.notifyAll();
            }
            configRequestMap.remove(jobid);
            return configRepo.getConfigYaml(jobid);
        }
    }

    @GetMapping("/yamlconfig/cancel")
    public String setYamlConfig(Long jobid, HttpServletRequest request,
                                HttpServletResponse response) {
        if (jobid == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }
        ExecutionCommand curMonitor = configRequestMap.get(jobid);
        if (curMonitor == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return null;
        } else {
            synchronized (curMonitor) {
                curMonitor.execute = false;
                curMonitor.notifyAll();
            }
            configRequestMap.remove(jobid);
            return configRepo.getConfigYaml(jobid);
        }
    }
}
