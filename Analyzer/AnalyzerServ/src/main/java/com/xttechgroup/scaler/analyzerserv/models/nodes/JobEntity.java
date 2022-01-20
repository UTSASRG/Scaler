package com.xttechgroup.scaler.analyzerserv.models.nodes;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ElfImgInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.relations.InvokeImg;
import org.springframework.data.neo4j.core.schema.*;

import java.util.ArrayList;
import java.util.List;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("Job")
public class JobEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Relationship(type = "HAS_IMGINFO", direction = OUTGOING)
    private List<ElfImgInfoEntity> elfInfos = new ArrayList<>();


    @Relationship(type = "HAS_TIMINGINFO", direction = OUTGOING)
    private List<InvokeImg> invokeImgs = new ArrayList<>();

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public List<ElfImgInfoEntity> getElfInfos() {
        return elfInfos;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof JobEntity) {
            return id == ((JobEntity) obj).id;
        } else {
            return false;
        }
    }
}
