package com.xttechgroup.scaler.analyzerserv.models.Job;

import org.springframework.data.neo4j.core.schema.GeneratedValue;
import org.springframework.data.neo4j.core.schema.Id;
import org.springframework.data.neo4j.core.schema.Node;

@Node("Job")
public class JobEntity {
    @Id
    @GeneratedValue
    public Long id;


}
