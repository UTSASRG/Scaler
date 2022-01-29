package com.xttechgroup.scaler.analyzerserv.controller.restful;

//
//@RestController
//@RequestMapping("/profiling")
//public class ProfilingInfoRestController {
//
//    @GetMapping()
//    public List<ElfImgInfoQueryResult> getJobCountingInfo(Long jobid, Long symPagingNum) {
//        if (symPagingNum == null) {
//            symPagingNum = 5;
//        }
//        List<ElfImgEntity> allImgs = elfImgRepo.getELFImgEntity(jobid);
//        List<ElfImgInfoQueryResult> result = new ArrayList<>(allImgs.size());
//        for (int i = 0; i < allImgs.size(); ++i) {
//            ElfImgEntity curImg = allImgs.get(i);
//            List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, curImg.id, symPagingNum, 0);
//            curImg.setSymbolsInThisFile(allSyms);
//            Long allSymCount = elfImgRepo.getAllSymSum(jobid, curImg.id);
//            Long hookedSymCount = elfImgRepo.getHookedSymSum(jobid, curImg.id);
//            result.add(new ElfImgInfoQueryResult(curImg, allSymCount, hookedSymCount));
//
//        }
//        return result;
//    }
//
//
//    @GetMapping("/symbol")
//    public List<ELFSymEntity> getELFSymbol(Long jobid, Long elfImgId, Integer symPagingStart, Integer symPagingNum,
//                                           HttpServletRequest request, HttpServletResponse response) {
//        if (jobid == null || elfImgId == null || symPagingStart == null || symPagingNum == null) {
//            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
//        }
//        List<ELFSymEntity> allSyms = elfSymRepo.getELFSyms(jobid, elfImgId, symPagingNum, symPagingStart);
//        return allSyms;
//    }
//
//
//}
