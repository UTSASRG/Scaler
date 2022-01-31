package com.xttechgroup.scaler.analyzerserv.models.nodes;

import com.xttechgroup.scaler.analyzerserv.models.relations.JobInvokeSym;
import org.springframework.data.neo4j.core.schema.*;

import java.util.ArrayList;
import java.util.List;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("Job")
public class JobEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Relationship(type = "HAS_IMG", direction = OUTGOING)
    private List<ElfImgEntity> elfInfos = new ArrayList<>();

    //Only stores counting info
    @Relationship(type = "JobInvokeSym", direction = OUTGOING)
    private List<JobInvokeSym> invokeSyms = new ArrayList<>();

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
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
