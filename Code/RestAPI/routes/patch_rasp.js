const express = require("express");

module.exports = (supabase) => {
    const router = express.Router();
    
    // Mapeamento de valores numéricos para status
    const STATUS_MAP = {
        0: "RED",
        1: "GREEN",
        2: "YELLOW"
    };
    
    router.patch("/data/:table", async (req, res) => {
        try {
            const { table } = req.params;       // table to update
            const { identifierField, identifierValue, updateField, updateValue } = req.body;
            
            if (!table || !identifierField || identifierValue < 0 || !updateField || updateValue === undefined) {
                return res.status(400).json({ error: 400, detail: "Missing required parameters" });
            }
            
            const allowedTables = ["t_semaphore", "p_semaphore"];
            if (!allowedTables.includes(table)) {
                return res.status(400).json({ error: 400, detail: "Table not allowed" });
            }
            
            const allowedUpdateFields = ["status"];
            if (!allowedUpdateFields.includes(updateField)) {
                return res.status(400).json({
                    error: 400,
                    detail: "Update field not allowed"
                });
            }
            
            if (identifierField === "location") {
                const { data: existsData, error: existsError } = await supabase
                    .from(table)
                    .select("id")
                    .eq("location", identifierValue)
                    .limit(1);
                
                if (existsError) {
                    return res.status(500).json({
                        error: 500,
                        detail: existsError.message
                    });
                }
                
                if (!existsData || existsData.length === 0) {
                    return res.status(404).json({
                        error: 404,
                        detail: "Record with given name not found"
                    });
                }
            }
            
            // Converter o valor numérico para string se for o campo "status"
            let finalUpdateValue = updateValue;
            if (updateField === "status" && typeof updateValue === "number") {
                if (!(updateValue in STATUS_MAP)) {
                    return res.status(400).json({
                        error: 400,
                        detail: "Invalid status value. Use 0 (RED), 1 (GREEN), or 2 (YELLOW)"
                    });
                }
                finalUpdateValue = STATUS_MAP[updateValue];
            }
            
            const updates = { [updateField]: finalUpdateValue };
            const { data, error } = await supabase
                .from(table)
                .update(updates)
                .eq(identifierField, identifierValue)
                .select();
            
            if (error) {
                return res.status(500).json({ error: 500, detail: error.message });
            }
            
            if (!data || data.length === 0) {
                return res.status(404).json({ error: 404, detail: "Record not found" });
            }
            
            return res.status(200).json(data[0]);
        } catch (err) {
            return res.status(500).json({ error: 500, detail: err.message });
        }
    });
    
    return router;
};