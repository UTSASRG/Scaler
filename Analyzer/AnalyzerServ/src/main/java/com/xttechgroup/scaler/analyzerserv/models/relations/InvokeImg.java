package com.xttechgroup.scaler.analyzerserv.models.relations;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ElfImgInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.timing.ELFImgTimingEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;
import org.springframework.data.neo4j.core.schema.TargetNode;

@RelationshipProperties
public class InvokeImg {

    @RelationshipId
    private Long id;

    @Property
    Long duration;

    @Property
    Long counts;

    @TargetNode
    ELFImgTimingEntity libInfo;

}
