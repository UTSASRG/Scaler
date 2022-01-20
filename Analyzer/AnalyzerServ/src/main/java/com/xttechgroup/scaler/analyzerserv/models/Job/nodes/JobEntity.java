package com.xttechgroup.scaler.analyzerserv.models.Job.nodes;

import org.springframework.data.neo4j.core.schema.*;

import java.util.HashSet;
import java.util.Set;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("Job")
public class JobEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Relationship(type = "HAS_IMG", direction = OUTGOING)
    private Set<ElfImgInfoEntity> elfInfos = new HashSet<>();

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public Set<ElfImgInfoEntity> getElfInfos() {
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
