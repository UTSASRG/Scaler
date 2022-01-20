package com.xttechgroup.scaler.analyzerserv.models.Job.relations;

import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.models.Job.TimingLibraryInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.nodes.ElfImgInfoEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;

import java.util.List;

@RelationshipProperties
public class InvokeLib {

    @RelationshipId
    private Long id;

    @Property
    Long duration;

    @Property
    Long counts;

    @Property
    ElfImgInfoEntity libInfo;

}
