package com.xttechgroup.scaler.analyzerserv.controller.restful;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import com.xttechgroup.scaler.analyzerserv.models.queries.ElfImgInfoQueryResult;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFImgRepo;
import com.xttechgroup.scaler.analyzerserv.models.repository.ELFSymRepo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.List;

@RestController
@RequestMapping("/elfInfo")
public class ElfInfoRestController {
    @Autowired
    ELFImgRepo elfImgRepo;
    @Autowired
    ELFSymRepo elfSymRepo;

    @GetMapping()
    public List<ElfImgInfoQueryResult> getELFImg(Long jobid, Integer symPagingNum) {
        if (symPagingNum == null) {
            symPagingNum = 5;
        }
        List<ElfImgInfoQueryResult> allImgs = elfImgRepo.getELFImgEntity(jobid);
        for (int i = 0; i < allImgs.size(); ++i) {
            ElfImgEntity curImg = allImgs.get(i).curImg;
            List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, curImg.id, symPagingNum, 0);
            curImg.setSymbolsInThisFile(allSyms);
        }
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
