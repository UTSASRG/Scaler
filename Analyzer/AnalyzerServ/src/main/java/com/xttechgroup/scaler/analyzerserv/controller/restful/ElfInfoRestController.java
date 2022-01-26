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

import java.util.List;

@RestController
@RequestMapping("/elfInfo")
public class ElfInfoRestController {
    @Autowired
    ELFImgRepo elfImgRepo;
    @Autowired
    ELFSymRepo elfSymRepo;

    @GetMapping()
    public String getELFImg(Long jobid, Integer symPagingNum) {
        if (symPagingNum == null) {
            symPagingNum = 5;
        }
        List<ElfImgInfoQueryResult> allImgs = elfImgRepo.getELFImgEntity(jobid);
        for (int i = 0; i < allImgs.size(); ++i) {
            ElfImgEntity curImg = allImgs.get(i).curImg;
            List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, curImg.id, symPagingNum);
            curImg.setSymbolsInThisFile(allSyms);
        }

        return "Yes";
    }

//    @GetMapping("/{jobid}/{imgid}")
//    public String getImgInfo(Long jobId,Long imgId) {
//        Optional<ElfImgEntity> elfRlt = elfImgRepo.findById((long)4);
//        return "Yes";
//    }
}
