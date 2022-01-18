package com.xttechgroup.scaler.analyzerserv.models.Job;

import org.springframework.data.annotation.Version;
import org.springframework.data.neo4j.core.schema.GeneratedValue;
import org.springframework.data.neo4j.core.schema.Id;
import org.springframework.data.neo4j.core.schema.Node;
import org.springframework.data.neo4j.core.schema.Relationship;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("Job")
public class JobEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Version
    public Long version = 1L;

    @Relationship(type = "HAS", direction = OUTGOING)
    private Set<ElfImgInfoEntity> elfInfos = new HashSet<>();

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public void setElfInfos(Set<ElfImgInfoEntity> elfInfos) {
        this.elfInfos = elfInfos;
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
