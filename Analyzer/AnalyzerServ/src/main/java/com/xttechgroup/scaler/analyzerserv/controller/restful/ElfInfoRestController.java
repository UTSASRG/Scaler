package com.xttechgroup.scaler.analyzerserv.controller.restful;

import com.xttechgroup.scaler.analyzerserv.models.POJO.MultipleElfIds;
import com.xttechgroup.scaler.analyzerserv.models.POJO.SymCountQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.POJO.ElfImgInfoQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFSymRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.JobInvokedSymRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.neo4j.core.Neo4jClient;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

@RestController
@RequestMapping("/elfInfo")
public class ElfInfoRestController {
    @Autowired
    ELFImgRepo elfImgRepo;
    @Autowired
    ELFSymRepo elfSymRepo;
    @Autowired
    JobInvokedSymRepo jobInvokedSymRepo;
    @Autowired
    Neo4jClient neo4jClient;

    @GetMapping("/all")
    public List<ElfImgInfoQueryResult> getAll(Long jobid, Integer symPagingNum) {
        if (symPagingNum == null) {
            symPagingNum = 5;
        }

        List<ElfImgEntity> allImgs = elfImgRepo.getELFImgEntity(jobid, null);
        List<ElfImgInfoQueryResult> result = new ArrayList<>(allImgs.size());
        for (int i = 0; i < allImgs.size(); ++i) {
            ElfImgEntity curImg = allImgs.get(i);
            List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, curImg.id, symPagingNum, 0);
            curImg.setSymbolsInThisFile(allSyms);
            Long allSymCount = elfImgRepo.getAllSymSum(jobid, curImg.id);
            Long hookedSymCount = elfImgRepo.getHookedSymSum(jobid, curImg.id);
            result.add(new ElfImgInfoQueryResult(curImg, allSymCount, hookedSymCount));
        }
        return result;
    }

    @GetMapping("/image")
    public List<ElfImgEntity> getELFImg(Long jobid, Boolean elfImgValid) {
        List<ElfImgEntity> allImgs = elfImgRepo.getELFImgEntity(jobid, elfImgValid);
        return allImgs;
    }

    @PostMapping("/image/counting")
    public List<Long> getELFImgCounting(Long jobid, @RequestBody MultipleElfIds body,
                                        HttpServletRequest request,
                                        HttpServletResponse response) {
        if (jobid == null || body == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }

        ArrayList<Long> countingRlt = new ArrayList<>();
        for (Long elfImgId : body.elfImgIds) {
            countingRlt.add(jobInvokedSymRepo.getELFImgCount(jobid, elfImgId));
        }
        return countingRlt;
    }

    @GetMapping("/image/counting/symbols")
    public Collection<SymCountQueryResult> getELFImgCountingSymbols(Long jobid, Long elfImgId,
                                                                    HttpServletRequest request,
                                                                    HttpServletResponse response) {
        if (jobid == null || elfImgId == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }

        return jobInvokedSymRepo.getELFImgCountSymbols(jobid, elfImgId);
    }

    @GetMapping("/symbol")
    public List<ELFSymEntity> getELFSymbol(Long jobid, Long elfImgId, Integer symPagingStart, Integer symPagingNum,
                                           HttpServletRequest request, HttpServletResponse response) {
        if (jobid == null || elfImgId == null || symPagingStart == null || symPagingNum == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
        }
        List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, elfImgId, symPagingNum, symPagingStart);

        return allSyms;
    }


}
