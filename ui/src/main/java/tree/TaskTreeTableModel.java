/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Matt Palmer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package tree;

import com.formdev.flatlaf.FlatIconColors;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Task;
import data.TaskState;
import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;

import javax.swing.*;
import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.TreeNode;
import java.util.Comparator;
import java.util.Objects;

public final class TaskTreeTableModel extends TreeTableModel {
    private FlatSVGIcon activeIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/activity-svgrepo-com.svg"))).derive(24, 24);
    private FlatSVGIcon finishIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/checkmark-svgrepo-com.svg"))).derive(24, 24);
    private FlatSVGIcon pendingIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/system-pending-line-svgrepo-com.svg"))).derive(24, 24);

    public TaskTreeTableModel(final TreeNode rootNode, final boolean showRoot) {
        super(rootNode, showRoot);
        setIcons();
        setGroupingComparator(Comparator.comparingInt(o -> ((Task) ((DefaultMutableTreeNode) o).getUserObject()).indexInParent));
    }

    @Override
    public Class<?> getColumnClass(final int columnIndex) {
        return String.class;
    }

    @Override
    public Object getColumnValue(final TreeNode node, final int column) {
        final Task obj = TreeUtils.getUserObject(node);
        switch (column) {
            case 0:
                return obj.name;
            default:
                return null;
        }
    }

    @Override
    public TableColumnModel createTableColumnModel() {
        TableColumnModel result = new DefaultTableColumnModel();
        result.addColumn(TableUtils.createColumn(0, "Description"));
        return result;
    }

    @Override
    public Icon getNodeIcon(TreeNode node) {
        if (node != null) {
            final Task obj = TreeUtils.getUserObject(node);

            if (obj.state == TaskState.FINISHED) {
                return finishIcon;
            } else if (obj.state == TaskState.ACTIVE) {
                return activeIcon;
            }
            return pendingIcon;
        }
        return null;
    }

    private void setIcons() {
        activeIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_GREEN.key)));
        finishIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_PURPLE.key)));
        pendingIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_YELLOW.key)));
    }
}
