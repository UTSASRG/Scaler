package com.xttechgroup.scaler.analyzerserv.models;

import com.xttechgroup.scaler.analyzerserv.models.relations.InvokeImg;
import com.xttechgroup.scaler.analyzerserv.models.relations.InvokeSym;
import org.springframework.data.neo4j.core.schema.*;

import java.util.List;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("CountingInfo")
public class TimingLibraryInfoEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Property
    public Long fileID;

    //Symbol level counting and timing
    @Relationship(type = "INVOKED_SYM", direction = OUTGOING)
    List<InvokeSym> invokedSymbol;

    //Library level counting and timing
    @Relationship(type = "INVOKED_LIB", direction = OUTGOING)
    List<InvokeImg> invokedLibrary;

}
