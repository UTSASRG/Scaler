package com.xttechgroup.scaler.analyzerserv.controller.restful;

import com.xttechgroup.scaler.analyzerserv.bean.HelloMsg;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.concurrent.atomic.AtomicLong;

@RestController
@RequestMapping("/info")
public class InfoRestController {
    private final String api = "rest";
    @Value("${app.name}")
    private String appName;
    @Value("${app.version}")
    private String version;
    private final String slogan = "You know, for performance.";

    @GetMapping
    public HelloMsg greeting() {
        return new HelloMsg(api, appName, version,slogan);
    }
}
