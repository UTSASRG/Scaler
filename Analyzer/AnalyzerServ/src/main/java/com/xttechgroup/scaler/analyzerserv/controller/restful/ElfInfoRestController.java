package com.xttechgroup.scaler.analyzerserv.controller.restful;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.queries.ElfImgInfoQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFSymRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.List;

@RestController
@RequestMapping("/elfInfo")
public class ElfInfoRestController {
    @Autowired
    ELFImgRepo elfImgRepo;
    @Autowired
    ELFSymRepo elfSymRepo;

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
